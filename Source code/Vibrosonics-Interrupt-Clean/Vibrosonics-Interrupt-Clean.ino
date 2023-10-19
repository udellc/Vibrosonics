/*/
########################################################
  This code asynchronously samples a signal via an interrupt timer to fill an input buffer. Once the input buffer is full, a fast fourier transform is performed using the arduinoFFT library.
  The data from FFT is then processed and a new signal is formed using a sine wave table and audio synthesis, which fills an output buffer that is then outputted asynchronously by the same
  interrupt timer used for sampling the signal.

  Code written by: Mykyta "Nick" Synytsia - synytsim@oregonstate.edu
  
  Special thanks to: Vincent Vaughn, Chet Udell, and Oleksiy Synytsia
########################################################
/*/

#include <arduinoFFTFloat.h>
#include <Arduino.h>
#include <driver/dac.h>
#include <driver/adc.h>
#include <math.h>

/*/
########################################################
  Directives
########################################################
/*/

  // audio sampling stuff
#define AUDIO_IN_PIN ADC1_CHANNEL_6 // corresponds to ADC on A2
#define AUDIO_OUT_PIN A0

#define FFT_WINDOW_SIZE 256   // size of a recording window, this value MUST be a power of 2
#define SAMPLING_FREQ 10000   // The audio sampling and audio output rate, with the ESP32 the fastest we could sample at is just under 24kHz, but
                              // bringing this down leaves more processing time for FFT, signal processing functions, and synthesis

#define FFT_MAX_AMP 300    // the maximum frequency magnitude of an FFT bin, this is multiplied by the total number of waves being synthesized for volume representation

  // the maximum number of peaks to look for with the findMajorPeaks() function, also corresponds to how many sine waves are being synthesized
#define MAX_NUM_PEAKS 16

#define DEBUG 0    // in debug mode, the time taken in loop is printed (in microseconds), along with the assigned sine wave frequencies and amplitudes

/*/
########################################################
  Other
########################################################
/*/

unsigned long loop_time = 0;  // used for printing time main loop takes to execute in debug mode

/*/
########################################################
  FFT
########################################################
/*/

const int sampleDelayTime = int(1000000 / SAMPLING_FREQ); // the time delay between sample

float vReal[FFT_WINDOW_SIZE];  // vReal is used for input at SAMPLING_FREQ and receives computed results from FFT
float vImag[FFT_WINDOW_SIZE];  // used to store imaginary values for computation

arduinoFFT FFT = arduinoFFT(vReal, vImag, FFT_WINDOW_SIZE, SAMPLING_FREQ);  // Object for performing FFT's


/*/
########################################################
  Stuff relevant to FFT signal processing
########################################################
/*/

// FFT_SIZE_BY_2 is FFT_WINDOW_SIZE / 2. We are sampling at double the frequency we are trying to detect, therefore only half of the FFT bins are used for analysis post FFT
const int FFT_WINDOW_SIZE_BY2 = int(FFT_WINDOW_SIZE) >> 1;

const float freqRes = float(SAMPLING_FREQ) / FFT_WINDOW_SIZE;  // the frequency resolution of FFT with the current window size

const float freqWidth = 1.0 / freqRes; // the width of an FFT bin

float freqs[FFT_WINDOW_SIZE_BY2];   // stores the data from the current fft window

int FFTPeaks[FFT_WINDOW_SIZE_BY2 >> 1]; // stores the peaks computed by FFT
float FFTPeaksAmp[FFT_WINDOW_SIZE_BY2 >> 1];

/*/
########################################################
  Variables used for generating an audio output
########################################################
/*/

const int AUD_IN_BUFFER_SIZE = FFT_WINDOW_SIZE;  
const int AUD_OUT_BUFFER_SIZE = FFT_WINDOW_SIZE * 2;
const int GEN_AUD_BUFFER_SIZE = FFT_WINDOW_SIZE * 3;

const float _SAMPLING_FREQ = 1.0 / SAMPLING_FREQ;

// a cosine wave for modulating sine waves
float cos_wave[AUD_OUT_BUFFER_SIZE];

// sine wave for storing pre-generated values of a sine wave at SAMPLING_FREQ sample rate
float sin_wave[SAMPLING_FREQ];

const int MAX_NUM_WAVES = MAX_NUM_PEAKS;
const int MAX_AMP_SUM = MAX_NUM_WAVES * FFT_MAX_AMP;
// stores the sine wave frequencies and amplitudes to synthesize
int sin_wave_frequency[MAX_NUM_WAVES];
int sin_wave_amplitude[MAX_NUM_WAVES];

// stores the position of sin_wave
int sin_wave_idx = 0;

// scratchpad array used for signal synthesis
float generateAudioBuffer[GEN_AUD_BUFFER_SIZE];
// stores the current index of the scratch pad audio output buffer
int generateAudioIdx = 0;
// used for copying final synthesized values from scratchpad audio output buffer to volatile audio output buffer
int generateAudioOutIdx = 0;

/*/
########################################################
  Variables used for interrupt and function to call when interrupt is triggered
########################################################
/*/

// used to store recorded samples for a gien window
volatile int AUD_IN_BUFFER[AUD_IN_BUFFER_SIZE];
volatile int AUD_IN_BUFFER_IDX = 0;

// rolling buffer for outputting synthesized signal
volatile int AUD_OUT_BUFFER[AUD_OUT_BUFFER_SIZE];
volatile int AUD_OUT_BUFFER_IDX = 0;

hw_timer_t *SAMPLING_TIMER = NULL;

// the procedure that is called when timer interrupt is triggered
void IRAM_ATTR ON_SAMPLING_TIMER(){
  if (AUD_IN_BUFFER_FULL()) { return; }
  OUTPUT_SAMPLE();
  RECORD_SAMPLE();
}

/*/
########################################################
  Setup
########################################################
/*/

void setup() {
  // set baud rate
  Serial.begin(115200);
  analogReadResolution(12);
  // wait for Serial to set up
  while (!Serial)
    ;
  delay(3000);
  Serial.println("Ready");

  // calculate values of cosine and sine wave at certain sampling frequency
  calculateCosWave();
  calculateSinWave();

  // setup pins
  pinMode(AUDIO_OUT_PIN, OUTPUT);

  Serial.println("Setup complete");

  delay(1000);

  // setup timer interrupt for audio sampling
  SAMPLING_TIMER = timerBegin(0, 80, true);                       // setting clock prescaler 1MHz (80MHz / 80)
  timerAttachInterrupt(SAMPLING_TIMER, &ON_SAMPLING_TIMER, true); // attach interrupt function
  timerAlarmWrite(SAMPLING_TIMER, sampleDelayTime, true);         // trigger interrupt every @sampleDelayTime microseconds
  timerAlarmEnable(SAMPLING_TIMER);                               // enabled interrupt
  timerStart(SAMPLING_TIMER);
}

/*/
########################################################
  Loop
########################################################
/*/

void loop() {
  if (AUD_IN_BUFFER_FULL()) {
    // in debug mode, print total time spent in loop, optionally interrupt timer can be disabled
    if (DEBUG) {
      // timerAlarmDisable(My_timer); // optionally disable interrupt timer during debug mode
      loop_time = micros();
    }

    // fft data processing, returns num sine waves to synthesize
    int numSineWaves = processData();

    // generate audio for the next audio window
    generateAudioForWindow(sin_wave_frequency, sin_wave_amplitude, numSineWaves);

    if (DEBUG) {
      Serial.print("time spent processing in microseconds: ");
      Serial.println(micros() - loop_time);
      
      Serial.print("(F, A): ");
      for (int i = 0; i < MAX_NUM_WAVES; i++) {
        Serial.printf("(%03d, %03d) ", sin_wave_frequency[i], sin_wave_amplitude[i]);
      }
      Serial.println();
      // timerAlarmEnable(My_timer);  // enable interrupt timer
    }
  }
}

/*/
########################################################
  Process data
########################################################
/*/

int processData() {
  // copy values from AUD_IN_BUFFER to vReal array
  setupFFT();
  
  // restores AUD_IN and AUD_OUT buffer positions
  RESET_AUD_IN_OUT_IDX();

  FFT.DCRemoval();                                  // DC Removal to reduce noise
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
  FFT.Compute(FFT_FORWARD);                         // Compute FFT
  FFT.ComplexToMagnitude();                         // Compute frequency magnitudes

  // copy values calculated by FFT to freqs
  storeFreqs();
  
  // noise flooring based on mean of data and a threshold
  noiseFloor(freqs, 20.0);

  // finds peaks in data, stores index of peak in FFTPeaks and Amplitude of peak in FFTPeaksAmp
  findMajorPeaks(freqs);
  
  // assigns data found by findMajorPeaks to sine waves
  int numSineWaves = assignSinWaves(FFTPeaks, FFTPeaksAmp, FFT_WINDOW_SIZE_BY2 >> 1);

  // convert from index to frequency and get the total sum of amplitudes
  int totalEnergy = 0;
  for (int i = 0; i < numSineWaves; i++) {
    totalEnergy += sin_wave_amplitude[i];
    sin_wave_frequency[i] = interpolateAroundPeak(freqs, sin_wave_frequency[i]);
  }

  // maps the amplitudes so that their total sum is no more or equal to 127
  mapAmplitudes(totalEnergy, numSineWaves);

  return numSineWaves;
}

/*/
########################################################
  Functions relating to assigning sine waves and mapping their frequencies and amplitudes
########################################################
/*/

// stores the frequencies and amplitudes found by majorPeaks into separate arrays, and returns the number of sin waves to synthesize
int assignSinWaves(int* freqData, float* ampData, int size) {
  // restore amplitudes to 0, restoring frequencies isn't necassary but is probably a good idea
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    sin_wave_amplitude[i] = 0;
    sin_wave_frequency[i] = 0;
  }
  int c = 0;
  // copy amplitudes that are above 0, otherwise skip
  for (int i = 0; i < size; i++) {
    // skip storing if ampData is 0, or freqData is 0
    if (ampData[i] == 0.0 || freqData[i] == 0) { continue; }
    sin_wave_amplitude[c] = round(ampData[i]);
    sin_wave_frequency[c++] = freqData[i];
    // break so no overflow occurs
    if (c == MAX_NUM_WAVES) { break; }
  }
  return c;
}

// maps amplitudes between 0 and 127 range to correspond to 8-bit ADC on ESP32 Feather
void mapAmplitudes(int totalEnergy, int numWaves) {
  // value to map amplitudes between 0.0 and 1.0 range, the MAX_AMP_SUM will be used to divide unless totalEnergy exceeds this value
  float divideBy = 1.0 / float(totalEnergy > MAX_AMP_SUM ? totalEnergy : MAX_AMP_SUM);

  // normalizing and multiplying to ensure that the sum of amplitudes is less than or equal to 127
  for (int i = 0; i < numWaves; i++) {
    int amplitude = sin_wave_amplitude[i];
    if (amplitude == 0) { continue; }
    // map amplitude between 0 and 127
    sin_wave_amplitude[i] = floor(amplitude * divideBy * 127.0);
  }
}

/*/
########################################################
  Functions related to FFT analysis
########################################################
/*/

// setup arrays for FFT analysis
void setupFFT() {
  for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
    // copy values from audio input buffer
    vReal[i] = AUD_IN_BUFFER[i];
    // set imaginary values to 0
    vImag[i] = 0.0;
  }
}

// store the current window into freqs and freqss for averaging windows to reduce noise and produce a cleaner spectrogram for signal processing
void storeFreqs () {
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    freqs[i] = vReal[i] * freqWidth;
  }
}

// noise flooring based on a set threshold
void noiseFloor(float *data, float threshold) {
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    if (data[i] < threshold) {
      data[i] = 0.0;
    }
  }
}

// finds all the peaks in the fft data* and removes the minimum peaks to contain output to @MAX_NUM_PEAKS
void findMajorPeaks(float* data) {
  // restore output arrays
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2 >> 1; i++) {
    FFTPeaks[i] = 0;
    FFTPeaksAmp[i] = 0;
  }
  // total sum of data
  int peaksFound = 0;
  float maxPeak = 0;
  int maxPeakIdx = 0;
  // iterate through data to find peaks
  for (int f = 1; f < FFT_WINDOW_SIZE_BY2 - 1; f++) {
    // determines if data[f] is a peak by comparing with previous and next location, otherwise continue
    if ((data[f - 1] < data[f]) && (data[f] > data[f + 1])) {
      if (data[f] > maxPeak) {
        maxPeak = data[f];
        maxPeakIdx = f;
      }
      // store sum around the peak and index of peak
      FFTPeaksAmp[peaksFound] = data[f - 1] + data[f] + data[f + 1];
      FFTPeaks[peaksFound++] = f;
    }
  }
  // if needed, remove a certain number of the minumum peaks to contain output to @maxNumberOfPeaks
  int numPeaksToRemove = peaksFound - MAX_NUM_PEAKS;
  for (int j = 0; j < numPeaksToRemove; j++) {
    // store minimum as the maximum peak
    int minimumPeak = maxPeak;
    int minimumPeakIdx = maxPeakIdx;
    // find the minimum peak and replace with zero
    for (int i = 1; i < peaksFound; i++) {
      int thisPeakAmplitude = FFTPeaksAmp[i];
      if (thisPeakAmplitude > 0 && thisPeakAmplitude < minimumPeak) {
        minimumPeak = thisPeakAmplitude;
        minimumPeakIdx = i;
      }
    }
    FFTPeaks[minimumPeakIdx] = 0;
    FFTPeaksAmp[minimumPeakIdx] = 0;
  }
}

// interpolation based on the weight of amplitudes around a peak
int interpolateAroundPeak(float *data, int indexOfPeak) {
  float prePeak = indexOfPeak == 0 ? 0.0 : data[indexOfPeak - 1];
  float atPeak = data[indexOfPeak];
  float postPeak = indexOfPeak == FFT_WINDOW_SIZE_BY2 ? 0.0 : data[indexOfPeak + 1];
  // summing around the index of maximum amplitude to normalize magnitudeOfChange
  float peakSum = prePeak + atPeak + postPeak;
  // interpolating the direction and magnitude of change, and normalizing from -1.0 to 1.0
  float magnitudeOfChange = ((atPeak + postPeak) - (atPeak + prePeak)) / (peakSum > 0.0 ? peakSum : 1.0);
  
  // return interpolated frequency
  return int(round((float(indexOfPeak) + magnitudeOfChange) * freqRes));
}

/*/
########################################################
  Functions related to generating values for the circular audio output buffer
########################################################
/*/

// calculate values for cosine function that is used for smoothing transition between frequencies (0.5 * (1 - cos((2PI / T) * x)), where T = AUD_OUT_BUFFER_SIZE
void calculateCosWave() {
  float resolution = float(2.0 * PI / AUD_OUT_BUFFER_SIZE);
  for (int i = 0; i < AUD_OUT_BUFFER_SIZE; i++) {
    cos_wave[i] = 0.5 * (1.0 - cos(float(resolution * i)));
  }
}

// calculate values for 1Hz sine wave @SAMPLING_FREQ sample rate
void calculateSinWave() {
  float resolution = float(2.0 * PI / SAMPLING_FREQ);
  for (int x = 0; x < SAMPLING_FREQ; x++) {
    sin_wave[x] = sin(float(resolution * x));
  }
}

// generates values for audio output buffer
void generateAudioForWindow(int *sin_waves_freq, int *sin_waves_amp, int num_sin_waves) {
  for (int i = 0; i < AUD_OUT_BUFFER_SIZE; i++) {
    // variable store the value of the sum of sine waves at this particular moment
    float sumOfSines = 0.0;
    // sum together the sine waves
    for (int s = 0; s < num_sin_waves; s++) {
      if (sin_waves_amp[s] == 0 || sin_waves_freq[s] == 0) { continue; }
      // calculate the sine wave index of the sine wave corresponding to the frequency
      //int sin_wave_position = (sin_wave_freq_idx) % int(SAMPLING_FREQ); // a slightly faster way of doing mod function to reduce usage of division
      float sin_wave_freq_idx = sin_wave_idx * sin_waves_freq[s] * _SAMPLING_FREQ;
      int sin_wave_position = (sin_wave_freq_idx - floor(sin_wave_freq_idx)) * SAMPLING_FREQ;
      sumOfSines += sin_waves_amp[s] * sin_wave[sin_wave_position];
    }
    // window sum of sines by cos wave at this moment in time
    float synthesized_value = cos_wave[i] * sumOfSines;
    // add to the existing values in scratch pad audio output buffer
    generateAudioBuffer[generateAudioIdx] += synthesized_value;

    // copy final, synthesized values to volatile audio output buffer
    if (i < AUD_IN_BUFFER_SIZE) {
      // shifting output by 128.0 for ESP32 DAC
      AUD_OUT_BUFFER[generateAudioOutIdx++] = generateAudioBuffer[generateAudioIdx] + 128.0;
      if (generateAudioOutIdx == AUD_OUT_BUFFER_SIZE) { generateAudioOutIdx = 0; }
    }

    // increment generate audio index
    generateAudioIdx += 1;
    if (generateAudioIdx == GEN_AUD_BUFFER_SIZE) { generateAudioIdx = 0; }
    // increment sine wave index
    sin_wave_idx += 1;
    if (sin_wave_idx == SAMPLING_FREQ) { sin_wave_idx = 0; }
  }

  // reset the next window to synthesize new signal
  int generateAudioIdxCpy = generateAudioIdx;
  for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
    generateAudioBuffer[generateAudioIdxCpy++] = 0.0;
    if (generateAudioIdxCpy == GEN_AUD_BUFFER_SIZE) { generateAudioIdxCpy = 0; }
  }
  // determine the next position in the sine wave table, and scratch pad audio output buffer to counter phase cosine wave
  generateAudioIdx = int(generateAudioIdx - FFT_WINDOW_SIZE + GEN_AUD_BUFFER_SIZE) % int(GEN_AUD_BUFFER_SIZE);
  sin_wave_idx = int(sin_wave_idx - FFT_WINDOW_SIZE + SAMPLING_FREQ) % int(SAMPLING_FREQ);
}

/*/
########################################################
  Functions related to sampling and outputting audio by interrupt
########################################################
/*/

// returns true if AUD_IN_BUFFER is full
bool AUD_IN_BUFFER_FULL() {
  return !(AUD_IN_BUFFER_IDX < FFT_WINDOW_SIZE);
}

// restores AUD_IN_BUFFER_IDX and AUD_OUT_BUFFER_IDX, and increments NUM_WINDOWS_REC to allow values to be properly synthesized for AUD_OUT_BUFFER
void RESET_AUD_IN_OUT_IDX() {
  // reset volatile audio input buffer position
  AUD_IN_BUFFER_IDX = 0;
  // reset volatile audio output buffer position, if needed
  if (AUD_OUT_BUFFER_IDX < AUD_OUT_BUFFER_SIZE) { return; }
  AUD_OUT_BUFFER_IDX = 0;
}

// outputs a sample from the volatile output buffer to DAC
void OUTPUT_SAMPLE() {
  dacWrite(AUDIO_OUT_PIN, AUD_OUT_BUFFER[AUD_OUT_BUFFER_IDX++]);
}

// records a sample from the ADC and stores into volatile input buffer
void RECORD_SAMPLE() {
  AUD_IN_BUFFER[AUD_IN_BUFFER_IDX++] = adc1_get_raw(AUDIO_IN_PIN);
}
