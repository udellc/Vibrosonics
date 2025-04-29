#include <vibrosonics.h>

AD56X4Class dac;
byte channel[] = { AD56X4_CHANNEL_A, AD56X4_CHANNEL_B, AD56X4_CHANNEL_C, AD56X4_CHANNEL_D };
hw_timer_t *SAMPLING_TIMER = NULL;
uint16_t sinusoid[SAMPLE_RATE] = {0};
double samples[FFT_SAMPLES]= {0};
double vReal[FFT_SAMPLES]= {0};
double vImag[FFT_SAMPLES]= {0};
volatile int frequency = 440;
volatile int FFTtimer = 0;
volatile int iterator = 0;
volatile int current_wave = 0;
volatile bool updateWave = false;

extern ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, FFT_SAMPLES, SAMPLE_RATE);

void initialize(void (*DAC_OUT)())
{
  const int sampleDelayTime = 1000000 / SAMPLE_RATE;

  Serial.begin(115200);
  delay(3000);
  Serial.println("\nSerial connection initiated.");

  Serial.println("Initializing SPI communication...")
  pinMode(33, OUTPUT);
  pinMode(SS_PIN, OUTPUT);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.begin();

  Serial.println("Initializing AD5644 DAC...")
  dac.reset(SS_PIN, true);
  dac.useInternalReference(SS_PIN, true);

  Serial.println("Configuring analog input...")
  pinMode(A2, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(A2, ADC_0db);

  Serial.println("Starting interrupt setup...");
  SAMPLING_TIMER = timerBegin(1000000); // setting clock prescaler 1MHz (80MHz / 80)
  timerAttachInterrupt(SAMPLING_TIMER, DAC_OUT); // attach interrupt function
  timerAlarm(SAMPLING_TIMER, sampleDelayTime, true, 0); // trigger interrupt every sampleDelayTime microseconds
  Serial.println("Interrupts initialized. Setup is complete.");
}

void generateWave(int waveFrequency, uint16_t sinusoid[SAMPLE_RATE], double volume) 
{
  if(waveFrequency == 0)
  {
    for(int i = 0 ; i < SAMPLE_RATE; i++)
      sinusoid[i] = 0;
  }

  float samplesPerCycle = (float)SAMPLE_RATE / waveFrequency;
  int totalCycles = round(SAMPLE_RATE / samplesPerCycle);
  float actualSamplesPerCycle = (float)SAMPLE_RATE / totalCycles;

  for (int i = 0; i < SAMPLE_RATE; i++)
  {
    float phase = (2 * M_PI * i) / actualSamplesPerCycle;
    sinusoid[i] = (uint16_t)(8192 * volume * (1 + sin(phase)));
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

  int cutoff = 100;
  int cutoff_bin = cutoff * FFT_SAMPLES / SAMPLE_RATE;
  double maxVal = NOISE_THRESHOLD;
  int maxIndex = -1;

  for (int i = cutoff_bin; i < FFT_SAMPLES / 2; i++) {
    if (vReal[i] > maxVal) 
    {
      maxVal = vReal[i];
      maxIndex = i;
    }
  }

  if(maxIndex >= 0)
  {
    double domFrequency = (maxIndex * SAMPLE_RATE) / FFT_SAMPLES;
    if(!isnan(domFrequency))
    {
      Serial.print((int)domFrequency); //Print out what frequency is the most dominant.
      Serial.print (" Hz | Magnitude: ");
      Serial.print(maxVal);
      Serial.print(" | Mapped to: ");
      domFrequency = mapFrequency(domFrequency);
      frequency = domFrequency;
      Serial.print(frequency);
      Serial.println(" Hz");
    } 
    FFTtimer = millis();
  }
  if(millis() - FFTtimer > 500)
    frequency = 0;
}

double mapFrequency(double inputFreq)
{
    const double input_min = 100.0;
    const double input_max = 2000.0;
    const double output_min = 20.0;
    const double output_max = 500.0;

    if (inputFreq < input_min) inputFreq = input_min;
    if (inputFreq > input_max) inputFreq = input_max;

    double outputFreq = output_min + (inputFreq - input_min) * (output_max - output_min) / (input_max - input_min);

    return outputFreq;
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

void speakerMode(byte channel)
{
  int val = (analogRead(A2) - 1852) * 10 + 8192; // The board's DC offset results in a raw value of about 1852. We normalize, incrase the magnitude, and then center at 8192 instead.
  dac.setChannel(SS_PIN, AD56X4_SETMODE_INPUT_DAC, channel, (word)val);
}

void FFTMode()
{
  if(iterator < FFT_SAMPLES)
  {
    samples[iterator] = analogRead(A2);
    dac.setChannel(SS_PIN, AD56X4_SETMODE_INPUT_DAC, AD56X4_CHANNEL_A, (word)(sinusoid[iterator++]));
  }
  else
  {
    iterator = 0;
    updateWave = true;
  }
}

void FFTMode_loop(double volume)
{
  if(updateWave)
  {
    analyzeWave();
    updateWave = false;
  }
  if(abs(current_wave - frequency) > 10)
  {
    generateWave(frequency, sinusoid, volume);
    current_wave = frequency;
  }
}
