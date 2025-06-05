#include <vibrosonics.h>

AD56X4Class dac; // DAC object
SI470X rx; // FM object
extern ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, FFT_SAMPLES, SAMPLE_RATE); // FFT Object
byte channel[] = { AD56X4_CHANNEL_A, AD56X4_CHANNEL_B, AD56X4_CHANNEL_C, AD56X4_CHANNEL_D }; // Bytes corresponding to our AD56X4's A, B, C, and D channels
hw_timer_t *SAMPLING_TIMER = NULL; // Interrupt timer
uint16_t midsWave[SAMPLE_RATE / 4] = {0}; // These two store our mids/highs sinusoid data. Because a given channel is outputted every four cycles, effective sample write is SAMPLE_RATE / 4.
uint16_t highsWave[SAMPLE_RATE / 4] = {0};
double samples[FFT_SAMPLES]= {0}; // Array where we collect samples; we do not write directly to vReal[] to avoid bins being overwritten
double vReal[FFT_SAMPLES]= {0}; // Stores "real" component of samples for analysis
double vImag[FFT_SAMPLES]= {0}; // Stores "imaginary" component of samples for analysis
volatile int midsFrequency = 0; // Output of mids frequency analysis
volatile int highsFrequency = 0; // Output of highs frequency analysis
volatile int FFTtimer = 0; // Keeps track of time since last new frequency for timeout purposes.
volatile int iterator = 0; // Used in the FFT Mode interupt to collect FFT_SAMPLES amount of samples before passing on that window for analysis and resetting to zero.
volatile int wave_iterator = 0; // Used when outputting waves to iterate through their sinusoid arrays
volatile int current_mids = 0; // Used to store current mids frequency so it can be compared against new ones
volatile int current_highs = 0; // Used to store current hgihs frequency so it can be compared against new ones
volatile bool updateWave = false; // This is made true when the interrupt collects a 512 sample window, triggering FFT analysis.

// Initialization of Serial and SPI communication, the AD56X4 DAC, analog input, and the interrupt.
void initialize(void (*DAC_OUT)(), bool FM)
{
  const int sampleDelayTime = 1000000 / SAMPLE_RATE;

  Serial.begin(115200);
  delay(3000);
  Serial.println("\nSerial connection initiated.");

  if(FM)
  {
    Serial.println("Initializing FM receiever...");
    initialize_FM();
  }

  Serial.println("Initializing SPI communication...");
  pinMode(SS_PIN, OUTPUT); // Setting slave select pin to output mode
  SPI.begin(); // Begins SPI communication

  Serial.println("Initializing AD56X4 DAC...");
  dac.reset(SS_PIN, true); // This is necessary! As outlined in AD56X4_vibrosonics.cpp, some important function calls were moved here for optimization purposes.
  dac.useInternalReference(SS_PIN, true); // Very necessary if leaving VREFOUT/VREFIN pin open on the AD56X4.

  Serial.println("Configuring analog input...");
  pinMode(A2, INPUT); // This code only takes input from A2 as stereo is not implemented, but A3 should also be initialized when using that for the other stereo input.
  analogReadResolution(12);
  analogSetPinAttenuation(A2, ADC_0db);

  Serial.println("Starting interrupt setup...");
  SAMPLING_TIMER = timerBegin(1000000); // Setting clock prescaler 1MHz (80MHz / 80)
  timerAttachInterrupt(SAMPLING_TIMER, DAC_OUT); // Attaching interrupt function
  timerAlarm(SAMPLING_TIMER, sampleDelayTime, true, 0); // Trigger interrupt every sampleDelayTime μs
  Serial.println("Interrupts initialized. Setup is complete.");
}

void initialize_FM()
{
  Wire.begin(SDA_PIN, SCL_PIN);
  rx.setup(RESET_PIN, SDA_PIN);
  rx.setVolume(FM_VOLUME);
  rx.setFrequency(FM_FREQUENCY);  // It is the frequency you want to select in MHz multiplied by 100.
  //Wire.end();
}

// Generates sinusoid at a specified frequency, in a given array, and at a specified volume
void generateWave(int waveFrequency, uint16_t sinusoid[SAMPLE_RATE / 4], double volume) 
{
  int out_rate = SAMPLE_RATE / 4; // With individual channels outputting once every four interrupts, we have an effective out_rate of SAMPLE_RATE / 4 (2kHz at 8kHz sample rate)

  if (waveFrequency > out_rate / 2) // Nyquist frequency; we cannot represent frequencies higher than the out_rate / 2 (1kHz at 8kHz sample rate)
    waveFrequency = 0;

  if(waveFrequency == 0) // This will effectively create a silent wave
  {
    for(int i = 0 ; i < out_rate / 4; i++)
      sinusoid[i] = 0;
  }

  float samplesPerCycle = (float)out_rate / waveFrequency; // We should reach 2π (one period) every samplesPerCycle indices, such that at the end of the loop we have reached 2π * waveFrequency

  for (int i = 0; i < out_rate; i++)
  {
    float phase = (2 * M_PI * i) / samplesPerCycle; // Ties the phase to our samplesPerCycle so that we reach 2π * waveFrequency at the end of the loop, allowing for clean repitition of the signal
    sinusoid[i] = (uint16_t)(volume * (pow(2, AD56X4_SIZE - 1) * (1 + sin(phase)))); // We generate a value corresponding with the current phase of our wave, amplified to half the total output size (13 bits for AD5644) and scaled with volume
  }
}

// Performs FFT analysis on our current window and identifies dominant mid and high range frequencies.
void analyzeWave()
{
  for(int i = 0; i < FFT_SAMPLES; i++)
  {
    vReal[i] = samples[i]; // Copying samples into our "real" array
    vImag[i] = 0; // "Imaginary" array is always zero
  }

  FFT.dcRemoval(); // Disregards DC offset
  FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);	// Weigh data
  FFT.compute(FFTDirection::Forward); // Compute FFT 
  FFT.complexToMagnitude(); // Compute magnitudes

  double maxMids = NOISE_THRESHOLD; // Set highest seen so far to the noise threshold; anything below will be disregarded
  double maxHighs = NOISE_THRESHOLD;

  int maxMids_index = -1; // Start at -1 so we can identify if no significant signal is found
  int maxHighs_index = -1;

  for(int i = getBin(MIDS_INPUT_MIN); i < FFT_SAMPLES / 2; i++) // We only need the first (real) half of the bins, and we don't need any bins with frequencies prior to the input minimum
  {
    double value = vReal[i];

    if(i <= getBin(MIDS_INPUT_MAX) && value > maxMids) // If we find a frequency in this range of bins which has a higher magnitude than what we've seen before, it's now the new max
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

  if(maxMids_index >= 0) // If we found a significant (above threshold) mids frequency
  {
    double domMids = (maxMids_index * SAMPLE_RATE) / FFT_SAMPLES; // Mapping bin to frequency

    Serial.print ("Mids: ");
    Serial.print((int)domMids);
    Serial.print (" Hz | Magnitude: ");
    Serial.print(maxMids);
    Serial.print(" | Mapped to: ");
    midsFrequency = mapFrequency(domMids, MIDS_INPUT_MIN, MIDS_INPUT_MAX, MIDS_OUTPUT_MIN, MIDS_OUTPUT_MAX); // Mapping audio to haptics
    Serial.print(midsFrequency);
    Serial.println(" Hz\n");

    FFTtimer = millis(); // Start timer since last significant frequency
  }

  if(maxHighs_index >= 0) // If we found a significant (above threshold) highs frequency
  {
    double domHighs = (maxHighs_index * SAMPLE_RATE) / FFT_SAMPLES; // Mapping bin to frequency

    Serial.print ("Highs: ");
    Serial.print((int)domHighs);
    Serial.print (" Hz | Magnitude: ");
    Serial.print(maxHighs);
    Serial.print(" | Mapped to: ");
    domHighs = mapFrequency(domHighs, HIGHS_INPUT_MIN, HIGHS_INPUT_MAX, HIGHS_OUTPUT_MIN, HIGHS_OUTPUT_MAX);  // Mapping audio to haptics
    highsFrequency = domHighs;
    Serial.print(highsFrequency);
    Serial.println(" Hz\n");

    FFTtimer = millis(); // Start timer since last significant frequency
  }

  if(millis() - FFTtimer > FFT_TIMEOUT) // If the timer passes a value FFT_TIMEOUT, we set the frequency to zero (muting both mids and highs)
  {
    midsFrequency = 0;
    highsFrequency = 0;
  }
}

// Debugging function, useful for seeing the raw values that are coming into your analog input. This helped identify the DC offset value which speakerMode() normalizes on
void obtain_raw_analog()
{
  uint16_t data[10000];
  int sum = 0;
  int highest = 0;
  int lowest = 100000;
  for(int i = 0; i < 10000; i++) // We get 10,000 data samples
  {
    data[i] = analogRead(A2);
    sum += data[i]; // We take the total sum (for the average)
    if(data[i] > highest) // We identify the highest value
      highest = data[i];
    if(data[i] < lowest) // We identify the lowest value
      lowest = data[i];
  }

  Serial.print("Average: ");
  Serial.println(sum / 10000);
  Serial.print("Highest: ");
  Serial.println(highest);
  Serial.print("Lowest: ");
  Serial.println(lowest);
}

// Maps analyzed frequencies down into the haptic range
double mapFrequency(double inputFreq, double input_min, double input_max, double output_min, double output_max)
{
    if (inputFreq < input_min) inputFreq = input_min;
    if (inputFreq > input_max) inputFreq = input_max;

    double outputFreq = output_min + (inputFreq - input_min) * (output_max - output_min) / (input_max - input_min);

    return outputFreq;
}

// Estimates the FFT bin of a frequency based on the window size and sample rate
int getBin(double frequency)
{
  return frequency * FFT_SAMPLES / SAMPLE_RATE;
}

// Interrupt function for Speaker Mode: simply reads the input signal and outputs it directly to one of our analog output channels
void speakerMode(byte channel)
{
  int val = (analogRead(A2) - 1852) * 10 + 8192; // The board's DC offset results in a raw value of about 1852. We normalize, incrase the magnitude, and then center at 8192 instead.
  dac.setChannel(SS_PIN, AD56X4_SETMODE_INPUT_DAC, channel, (word)val);
}

// Interrupt function for FFT Mode: performs FFT analysis on the input signal to generate mids and highs haptic frequencies that are outputted to all four of our analog output channels
void FFTMode()
{
  if(iterator < 512) // We take 512 samples
    samples[iterator++] = analogRead(A2);
  else
  {
    iterator = 0; // Once we reach 512, we reset and tell the loop function to start analyzing the window we just collected
    updateWave = true;
  }
  static uint16_t current_channel = 0; // Static, so this only runs once

  uint16_t value = (current_channel < 2) ? midsWave[wave_iterator] : highsWave[wave_iterator]; // If we're on 0 or 1, output mids. If we're on 1 or 2, output highs.

  dac.setChannel(SS_PIN, AD56X4_SETMODE_INPUT_DAC, channel[current_channel], (word)(value)); // Output chosen value to current channel

  current_channel = (current_channel + 1) % 4; // Rotate between channels 0, 1, 2, and 3 (AKA A, B, C, D)

  if(current_channel == 0) // Only when we loop back around to 0 do we update the wave iterator, so that each signal gets all SAMPLE_RATE / 4 values outputted
    wave_iterator = (wave_iterator + 1) % 2048;
}

// Loop function for FFT Mode: triggers analysis when new sample windows are generated, generates sinusoid data for new frequencies
void FFTMode_loop(double volume)
{
  if(updateWave) // If our interrupt has told us that it has collected a full window
  {
    analyzeWave(); // Analyze the wave and reset the window signal
    updateWave = false;
  }
  if(abs(current_mids - midsFrequency) > 10) // Threshold avoids redundant frequency generation
  {
    generateWave(midsFrequency, midsWave, volume); // Generate a wave at the given frequency, in the midsWave[] array, at the volume specified in vibrosonics_operating_code.ino
    current_mids = midsFrequency;
  }
  if(abs(current_highs - highsFrequency) > 10) // Threshold avoids redundant frequency generation
  {
    generateWave(highsFrequency, highsWave, volume); // Generate a wave at the given frequency, in the midsWave[] array, at the volume specified in vibrosonics_operating_code.ino
    current_highs = highsFrequency;
  }
}
