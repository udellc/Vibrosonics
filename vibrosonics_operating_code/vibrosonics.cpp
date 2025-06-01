#include <vibrosonics.h>

AD56X4Class dac;
byte channel[] = { AD56X4_CHANNEL_A, AD56X4_CHANNEL_B, AD56X4_CHANNEL_C, AD56X4_CHANNEL_D };
hw_timer_t *SAMPLING_TIMER = NULL;
uint16_t midsWave[SAMPLE_RATE / 4] = {0};
uint16_t highsWave[SAMPLE_RATE / 4] = {0};
double samples[FFT_SAMPLES]= {0};
double vReal[FFT_SAMPLES]= {0};
double vImag[FFT_SAMPLES]= {0};
volatile int midsFrequency = 0;
volatile int highsFrequency = 0;
volatile int FFTtimer = 0;
volatile int iterator = 0;
volatile int wave_iterator = 0;
volatile int current_mids = 0;
volatile int current_highs = 0;
volatile bool updateWave = false;

extern ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, FFT_SAMPLES, SAMPLE_RATE);

void initialize(void (*DAC_OUT)())
{
  const int sampleDelayTime = 1000000 / SAMPLE_RATE;

  Serial.begin(115200);
  delay(3000);
  Serial.println("\nSerial connection initiated.");

  Serial.println("Initializing SPI communication...");
  pinMode(33, OUTPUT);
  pinMode(SS_PIN, OUTPUT);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.begin();

  Serial.println("Initializing AD5644 DAC...");
  dac.reset(SS_PIN, true);
  dac.useInternalReference(SS_PIN, true);

  Serial.println("Configuring analog input...");
  pinMode(A2, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(A2, ADC_0db);

  Serial.println("Starting interrupt setup...");
  SAMPLING_TIMER = timerBegin(1000000); // setting clock prescaler 1MHz (80MHz / 80)
  timerAttachInterrupt(SAMPLING_TIMER, DAC_OUT); // attach interrupt function
  timerAlarm(SAMPLING_TIMER, sampleDelayTime, true, 0); // trigger interrupt every sampleDelayTime microseconds
  Serial.println("Interrupts initialized. Setup is complete.");
}

void generateWave(int waveFrequency, uint16_t sinusoid[SAMPLE_RATE / 4], double volume) 
{
  int out_rate = SAMPLE_RATE / 4;

  if (waveFrequency > out_rate / 2) // nyquist frequency
    waveFrequency = 0;

  if(waveFrequency == 0)
  {
    for(int i = 0 ; i < out_rate / 4; i++)
      sinusoid[i] = 0;
  }

  float samplesPerCycle = (float)out_rate / waveFrequency;
  int totalCycles = round(out_rate / samplesPerCycle);
  float actualSamplesPerCycle = (float)out_rate / totalCycles;

  for (int i = 0; i < out_rate; i++)
  {
    float phase = (2 * M_PI * i) / actualSamplesPerCycle;
    sinusoid[i] = (uint16_t)(volume * (8192 * (1 + sin(phase))));
  }
}

void analyzeWave()
{
  for(int i = 0; i < FFT_SAMPLES; i++)
  {
    vReal[i] = samples[i];
    vImag[i] = 0;
  }

  FFT.dcRemoval();
  FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);	/* Weigh data */
  FFT.compute(FFTDirection::Forward); /* Compute FFT */
  FFT.complexToMagnitude(); /* Compute magnitudes */

  double maxMids = NOISE_THRESHOLD;
  double maxHighs = NOISE_THRESHOLD;

  int maxMids_index = -1;
  int maxHighs_index = -1;

  for(int i = getBin(MIDS_INPUT_MIN); i < FFT_SAMPLES / 2; i++)
  {
    double value = vReal[i];

    if(i <= getBin(MIDS_INPUT_MAX) && value > maxMids)
    {
      maxMids = value;
      maxMids_index = i;
    }
    else if (i >= getBin(HIGHS_INPUT_MIN) && i <= getBin(HIGHS_INPUT_MAX) && value > maxHighs)
    {
      maxHighs = value;
      maxHighs_index = i;
    }
  }

  if(maxMids_index >= 0)
  {
    double domMids = (maxMids_index * SAMPLE_RATE) / FFT_SAMPLES;
    Serial.print ("Mids: ");
    Serial.print((int)domMids);
    Serial.print (" Hz | Magnitude: ");
    Serial.print(maxMids);
    Serial.print(" | Mapped to: ");
    midsFrequency = mapFrequency(domMids, MIDS_INPUT_MIN, MIDS_INPUT_MAX, MIDS_OUTPUT_MIN, MIDS_OUTPUT_MAX);
    Serial.print(midsFrequency);
    Serial.println(" Hz\n");

    FFTtimer = millis();
  }

  if(maxHighs_index >= 0)
  {
    double domHighs = (maxHighs_index * SAMPLE_RATE) / FFT_SAMPLES;

    Serial.print ("Highs: ");
    Serial.print((int)domHighs);
    Serial.print (" Hz | Magnitude: ");
    Serial.print(maxHighs);
    Serial.print(" | Mapped to: ");
    domHighs = mapFrequency(domHighs, HIGHS_INPUT_MIN, HIGHS_INPUT_MAX, HIGHS_OUTPUT_MIN, HIGHS_OUTPUT_MAX);
    highsFrequency = domHighs;
    Serial.print(highsFrequency);
    Serial.println(" Hz\n");

    FFTtimer = millis();
  }

  if(millis() - FFTtimer > 500)
  {
    midsFrequency = 0;
    highsFrequency = 0;
  }
}


void obtain_raw_analog()
{
  uint16_t data[10000];
  int sum = 0;
  int highest = 0;
  int lowest = 100000;
  for(int i = 0; i < 10000; i++)
  {
    data[i] = analogRead(A2);
    sum += data[i];
    if(data[i] > highest)
      highest = data[i];
    if(data[i] < lowest)
      lowest = data[i];
  }

  Serial.print("Average: ");
  Serial.println(sum / 10000);
  Serial.print("Highest: ");
  Serial.println(highest);
  Serial.print("Lowest: ");
  Serial.println(lowest);
}

double mapFrequency(double inputFreq, double input_min, double input_max, double output_min, double output_max)
{
    if (inputFreq < input_min) inputFreq = input_min;
    if (inputFreq > input_max) inputFreq = input_max;

    double outputFreq = output_min + (inputFreq - input_min) * (output_max - output_min) / (input_max - input_min);

    return outputFreq;
}

int getBin(double frequency)
{
  return frequency * FFT_SAMPLES / SAMPLE_RATE;
}

void speakerMode(byte channel)
{
  int val = (analogRead(A2) - 1852) * 10 + 8192; // The board's DC offset results in a raw value of about 1852. We normalize, incrase the magnitude, and then center at 8192 instead.
  dac.setChannel(SS_PIN, AD56X4_SETMODE_INPUT_DAC, channel, (word)val);
}

void FFTMode()
{
  if(iterator < 512)
    samples[iterator++] = analogRead(A2);
  else
  {
    iterator = 0;
    updateWave = true;
  }
  static uint16_t current_channel = 0;

  uint16_t value = (current_channel < 2) ? midsWave[wave_iterator] : highsWave[wave_iterator];

  dac.setChannel(SS_PIN, AD56X4_SETMODE_INPUT_DAC, channel[current_channel], (word)(value));

  current_channel = (current_channel + 1) % 4;

  if(current_channel == 0)
    wave_iterator = (wave_iterator + 1) % 2048;
}

void FFTMode_loop(double volume)
{
  if(updateWave)
  {
    analyzeWave();
    updateWave = false;
  }
  if(abs(current_mids - midsFrequency) > 10)
  {
    generateWave(midsFrequency, midsWave, volume);
    current_mids = midsFrequency;
  }
  if(abs(current_highs - highsFrequency) > 10)
  {
    generateWave(highsFrequency, highsWave, volume);
    current_highs = highsFrequency;
  }
}
