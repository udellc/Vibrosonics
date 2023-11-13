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
#define AUD_IN_PIN ADC1_CHANNEL_6 // corresponds to ADC on A2
#define AUD_OUT_PIN A0

#define FFT_WINDOW_SIZE 256   // size of a recording window, this value MUST be a power of 2
#define SAMPLING_FREQ 10000   // The audio sampling and audio output rate, with the ESP32 the fastest we could sample at is just under 24kHz, but
                              // bringing this down leaves more processing time for FFT, signal processing functions, and synthesis

#define FFT_MAX_AMP 300   // the maximum frequency magnitude of an FFT bin, this is multiplied by the total number of waves being synthesized for volume representation

#define MAX_NUM_PEAKS 8  // the maximum number of peaks to look for with the findMajorPeaks() function, also corresponds to how many sine waves are being synthesized

#define MAX_NUM_WAVES 8  // maximum number of waves to synthesize (all channels combined)
#define AUD_OUT_CH 1  // number of audio channels to synthesize

#define DEBUG 0    // in debug mode, the time taken in loop is printed (in microseconds), along with the assigned sine wave frequencies and amplitudes

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

const int MAX_AMP_SUM = MAX_NUM_WAVES * FFT_MAX_AMP;

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
static float cos_wave_w[AUD_OUT_BUFFER_SIZE];

// sine wave for storing pre-generated values of a sine wave at SAMPLING_FREQ sample rate
static float sin_wave[SAMPLING_FREQ];
// float cos_wave[SAMPLING_FREQ];
// float tri_wave[SAMPLING_FREQ];
// float sqr_wave[SAMPLING_FREQ];
// float saw_wave[SAMPLING_FREQ];

// stores the sine wave frequencies and amplitudes to synthesize
static int sin_waves_freq[AUD_OUT_CH][MAX_NUM_WAVES];
static int sin_waves_amp[AUD_OUT_CH][MAX_NUM_WAVES];

// stores the number of sine waves in each channel
static int num_waves[AUD_OUT_CH];

// stores the position of wave
static int sin_wave_idx = 0;

// scratchpad array used for signal synthesis
static float generateAudioBuffer[AUD_OUT_CH][GEN_AUD_BUFFER_SIZE];
// stores the current index of the scratch pad audio output buffer
static int generateAudioIdx = 0;
// used for copying final synthesized values from scratchpad audio output buffer to volatile audio output buffer
static int generateAudioOutIdx = 0;

/*/
########################################################
  Variables used for interrupt and function to call when interrupt is triggered
########################################################
/*/

// used to store recorded samples for a gien window
volatile static int AUD_IN_BUFFER[AUD_IN_BUFFER_SIZE];
// rolling buffer for outputting synthesized signal
volatile static int AUD_OUT_BUFFER[AUD_OUT_CH][AUD_OUT_BUFFER_SIZE];

// audio input and output buffer index
volatile static int AUD_IN_BUFFER_IDX = 0;
volatile static int AUD_OUT_BUFFER_IDX = 0;

hw_timer_t *SAMPLING_TIMER = NULL;

// the procedure that is called when timer interrupt is triggered
void IRAM_ATTR ON_SAMPLING_TIMER(){
  AUD_IN_OUT();
}

/*/
########################################################
  Other
########################################################
/*/

unsigned long loop_time = 0;  // used for printing time main loop takes to execute in debug mode

const int bassIdx = 200 * freqWidth; // FFT bin index of frequency ~200Hz

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

  // initialize generate audio and output buffers to 0, not necassary but helps prevent glitches when program boots
  for (int i = 0; i < GEN_AUD_BUFFER_SIZE; i++) {
    for (int c = 0; c < AUD_OUT_CH; c++) {
      generateAudioBuffer[c][i] = 0.0;
      if (i < AUD_OUT_BUFFER_SIZE) {
        AUD_OUT_BUFFER[c][i] = 128;
      }
    }
  }

  // calculate values of cosine and sine wave at certain sampling frequency
  calculateWindowingWave();
  calculateWaves();
  resetSinWaves(0);

  // setup pins
  pinMode(AUD_OUT_PIN, OUTPUT);

  Serial.println("Setup complete");

  delay(1000);

  // setup timer interrupt for audio sampling
  SAMPLING_TIMER = timerBegin(0, 80, true);                       // setting clock prescaler 1MHz (80MHz / 80)
  timerAttachInterrupt(SAMPLING_TIMER, &ON_SAMPLING_TIMER, true); // attach interrupt function
  timerAlarmWrite(SAMPLING_TIMER, sampleDelayTime, true);         // trigger interrupt every @sampleDelayTime microseconds
  timerAlarmEnable(SAMPLING_TIMER);                               // enabled interrupt
  timerRestart(SAMPLING_TIMER);
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
      // timerAlarmDisable(SAMPLING_TIMER); // optionally disable interrupt timer during debug mode
      loop_time = micros();
    }

    // fft and data processing
    processData();

    // use data from FFT
    resetSinWaves(0);
    assignSinWaves(FFTPeaks, FFTPeaksAmp, FFT_WINDOW_SIZE_BY2 >> 1);
    mapAmplitudes();

    // synthesize 30Hz
    // resetSinWaves(0);
    // addSinWave(30, 127, 0);

    // generate audio for the next audio window
    generateAudioForWindow();

    if (DEBUG) {
      Serial.print("time spent processing in microseconds: ");
      Serial.println(micros() - loop_time);
      
      for (int c = 0; c < AUD_OUT_CH; c++) {
        Serial.printf("CH %d (F, A): ", c);
        for (int i = 0; i < num_waves[c]; i++) {
          Serial.printf("(%03d, %03d) ", sin_waves_freq[c][i], sin_waves_amp[c][i]);
        }
        Serial.println();
      }
      // timerAlarmEnable(SAMPLING_TIMER);  // enable interrupt timer
    }
  }
}

/*/
########################################################
  Process data
########################################################
/*/

// FFT data processing
void processData() {
  // copy values from AUD_IN_BUFFER to vReal array
  setupFFT();
  
  // reset AUD_IN_BUFFER_IDX to 0, so interrupt can continue to perform audio input and output
  RESET_AUD_IN_IDX();

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
}

/*/
########################################################
  Functions relating to assigning sine waves and mapping their frequencies and amplitudes
########################################################
/*/

// assigns the frequencies and amplitudes found by majorPeaks to sine waves
void assignSinWaves(int* freqData, float* ampData, int size) {  
  // assign sin_waves and freq/amps that are above 0, otherwise skip
  for (int i = 0; i < size; i++) {
    // skip storing if ampData is 0, or freqData is 0
    if (ampData[i] == 0.0 || freqData[i] == 0) continue;
    // assign frequencies below bass to left channel, otherwise to right channel
    if (freqData[i] <= bassIdx) {
      int freq = freqData[i] * freqRes;
      // if the difference of energy around the peak is greater than threshold
      if (abs(freqs[freqData[i] - 1] - freqs[freqData[i] + 1]) > 100) {
        // assign frequency based on whichever side is greater
        freq = freqs[freqData[i] - 1] > freqs[freqData[i] + 1] ? (freqData[i] - 0.5) * freqRes : (freqData[i] + 0.5) * freqRes;
      }
      addSinWave(freq, ampData[i], 0);
    } else {
      int interpFreq = interpolateAroundPeak(freqs, freqData[i]);
      addSinWave(interpFreq, ampData[i], 0);
    }
  }
}

// maps amplitudes between 0 and 127 range to correspond to 8-bit DAC on ESP32 Feather
void mapAmplitudes() {
  // map amplitudes on both channels
  for (int c = 0; c < AUD_OUT_CH; c++) {
    int ampSum = 0;
    for (int i = 0; i < num_waves[c]; i++) {
      int amplitude = sin_waves_amp[c][i];
      if (amplitude == 0) continue;
      ampSum += amplitude;
    }
    // since all amplitudes are 0, then there is no need to map between 0-127 range.
    if (ampSum == 0) { 
      num_waves[c] = 0;
      continue; 
    }
    // value to map amplitudes between 0.0 and 1.0 range, the MAX_AMP_SUM will be used to divide unless totalEnergy exceeds this value
    float divideBy = 1.0 / float(ampSum > MAX_AMP_SUM ? ampSum : MAX_AMP_SUM);
    ampSum = 0;
    // map all amplitudes between 0 and 128
    for (int i = 0; i < num_waves[c]; i++) {
      int amplitude = sin_waves_amp[c][i];
      if (amplitude == 0) continue;
      sin_waves_amp[c][i] = round(amplitude * divideBy * 127.0);
      ampSum += amplitude;
    }
    // ensures that nothing gets synthesized for this channel
    if (ampSum == 0) { num_waves[c] = 0; }
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

// store the current window into freqs
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
      float peakSum = data[f - 1] + data[f] + data[f + 1];
      if (peakSum > maxPeak) {
        maxPeak = peakSum;
        maxPeakIdx = f;
      }
      // store sum around the peak and index of peak
      FFTPeaksAmp[peaksFound] = peakSum;
      FFTPeaks[peaksFound++] = f;
    }
  }
  // if needed, remove a certain number of the minumum peaks to contain output to @maxNumberOfPeaks
  int numPeaksToRemove = peaksFound - MAX_NUM_PEAKS;
  for (int j = 0; j < numPeaksToRemove; j++) {
    // store minimum as the maximum peak
    float minimumPeak = maxPeak;
    int minimumPeakIdx = maxPeakIdx;
    // find the minimum peak and replace with zero
    for (int i = 0; i < peaksFound; i++) {
      float thisPeakAmplitude = FFTPeaksAmp[i];
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

// calculate values for cosine function that is used for smoothing transition between frequencies and amplitudes (0.5 * (1 - cos((2PI / T) * x)), where T = AUD_OUT_BUFFER_SIZE
void calculateWindowingWave() {
  float resolution = float(2.0 * PI / AUD_OUT_BUFFER_SIZE);
  for (int i = 0; i < AUD_OUT_BUFFER_SIZE; i++) {
    cos_wave_w[i] = 0.5 * (1.0 - cos(float(resolution * i)));
  }
}

// calculate values for 1Hz sine wave @SAMPLING_FREQ sample rate
void calculateWaves() {
  float resolution = float(2.0 * PI / SAMPLING_FREQ);
  // float tri_wave_step = 4.0 / SAMPLING_FREQ;
  // float saw_wave_step = 1.0 / SAMPLING_FREQ;
  for (int x = 0; x < SAMPLING_FREQ; x++) {
    sin_wave[x] = sin(float(resolution * x));
    // cos_wave[x] = cos(float(resolution * x));
    // tri_wave[x] = x <= SAMPLING_FREQ / 2 ? x * tri_wave_step - 1.0 : 3.0 - x * tri_wave_step;
    // sqr_wave[x] = x <= SAMPLING_FREQ / 2 ? 1.0 : 0.0;
    // saw_wave[x] = x * saw_wave_step;
  }
}

// assigns a sine wave to specified channel
void addSinWave(int freq, int amp, int ch) {
  if (ch > AUD_OUT_CH - 1) {
    if (DEBUG) Serial.printf("CANNOT ADD SINE WAVE, CHANNEL %d ISN'T DEFINED\n", ch);
    return;
  }
  // ensures that the sum of waves in the channels is no greater than MAX_NUM_WAVES
  int num_waves_count = 0;
  for (int c = 0; c < AUD_OUT_CH; c++) {
    num_waves_count += num_waves[c];
  }

  if (num_waves_count == MAX_NUM_WAVES) {
    if (DEBUG) Serial.println("CANNOT ASSIGN MORE WAVES, CHANGE MAX_NUM_WAVES!");
    return;
  }

  sin_waves_amp[ch][num_waves[ch]] = amp;
  sin_waves_freq[ch][num_waves[ch]] = freq;
  num_waves[ch] += 1;
}

// removes a sine wave at specified index and channel
void removeSinWave(int idx, int ch) {
  if (ch > AUD_OUT_CH - 1) {
    if (DEBUG) Serial.printf("CANNOT REMOVE WAVE, CHANNEL %d ISN'T DEFINED\n", ch);
    return;
  }
  if (idx >= num_waves[ch]) {
    if (DEBUG) Serial.printf("CANNOT REMOVE WAVE, INDEX %d DOESN'T EXIST!\n", idx);
    return;
  }

  for (int i = idx; i < --num_waves[ch]; i++) {
    sin_waves_amp[ch][i] = sin_waves_amp[ch][i + 1];
  }
}

// modify sine wave at specified index and channel to desired frequency and amplitude
void modifySinWave(int idx, int ch, int freq, int amp) {
  if (ch > AUD_OUT_CH - 1) {
    if (DEBUG) Serial.printf("CANNOT MODIFY WAVE, CHANNEL %d ISN'T DEFINED\n", ch);
    return;
  }
  if (idx >= num_waves[ch]) {
    if (DEBUG) Serial.printf("CANNOT MODIFY WAVE, INDEX %d DOESN'T EXIST!\n", idx);
    return;
  }
  sin_waves_amp[ch][idx] = amp;
  sin_waves_freq[ch][idx] = freq;
}

// sets all sine waves and frequencies to 0 on specified channel
void resetSinWaves(int ch) {
  if (ch > AUD_OUT_CH - 1) {
    if (DEBUG) Serial.printf("CANNOT RESET WAVES, CHANNEL %d ISN'T DEFINED\n", ch);
    return;
  }
  // restore amplitudes and frequencies on ch
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    sin_waves_amp[ch][i] = 0;
    sin_waves_freq[ch][i] = 0;
  }
  num_waves[ch] = 0;
}

// returns value of sine wave at given frequency and amplitude
float get_sin_wave_val(int freq, int amp) {
  float sin_wave_freq_idx = sin_wave_idx * freq * _SAMPLING_FREQ;
  int sin_wave_position = (sin_wave_freq_idx - floor(sin_wave_freq_idx)) * SAMPLING_FREQ;
  return amp * sin_wave[sin_wave_position];
}

// returns sum of sine waves of given channel
float get_sum_of_channel(int ch) {
  float sum = 0.0;
  for (int s = 0; s < num_waves[ch]; s++) {
    sum += get_sin_wave_val(sin_waves_freq[ch][s], sin_waves_amp[ch][s]);
  }
  return sum;
}

// generates values for one window of audio output buffer
void generateAudioForWindow() {
  for (int i = 0; i < AUD_OUT_BUFFER_SIZE; i++) {
    // sum together the sine waves for left channel and right channel
    for (int c = 0; c < AUD_OUT_CH; c++) {
      // add windowed value to the existing values in scratch pad audio output buffer at this moment in time
      generateAudioBuffer[c][generateAudioIdx] += get_sum_of_channel(c) * cos_wave_w[i];
    }
    // copy final, synthesized values to volatile audio output buffer
    if (i < AUD_IN_BUFFER_SIZE) {
      // shifting output by 128.0 for ESP32 DAC, min max ensures the value stays between 0 - 255 to ensure clipping won't occur
      for (int c = 0; c < AUD_OUT_CH; c++) {
        AUD_OUT_BUFFER[c][generateAudioOutIdx] = round(generateAudioBuffer[c][generateAudioIdx] + 128.0);
      }
      generateAudioOutIdx += 1;
      if (generateAudioOutIdx == AUD_OUT_BUFFER_SIZE) generateAudioOutIdx = 0;
    }
    // increment generate audio index
    generateAudioIdx += 1;
    if (generateAudioIdx == GEN_AUD_BUFFER_SIZE) generateAudioIdx = 0;
    // increment sine wave index
    sin_wave_idx += 1;
    if (sin_wave_idx == SAMPLING_FREQ) sin_wave_idx = 0;
  }

  // reset the next window to synthesize new signal
  int generateAudioIdxCpy = generateAudioIdx;
  for (int i = 0; i < AUD_IN_BUFFER_SIZE; i++) {
    for (int c = 0; c < AUD_OUT_CH; c++) {
      generateAudioBuffer[c][generateAudioIdxCpy] = 0.0;
    }
    generateAudioIdxCpy += 1;
    if (generateAudioIdxCpy == GEN_AUD_BUFFER_SIZE) generateAudioIdxCpy = 0;
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

// returns true if audio input buffer is full
bool AUD_IN_BUFFER_FULL() {
  return !(AUD_IN_BUFFER_IDX < AUD_IN_BUFFER_SIZE);
}

// restores AUD_IN_BUFFER_IDX, and increments AUD_OUT_COUNT to synchronize AUD_OUT_BUFFER
void RESET_AUD_IN_IDX() {
  AUD_OUT_BUFFER_IDX += AUD_IN_BUFFER_SIZE;
  if (AUD_OUT_BUFFER_IDX == AUD_OUT_BUFFER_SIZE) AUD_OUT_BUFFER_IDX = 0;
  AUD_IN_BUFFER_IDX = 0;
}

// outputs sample from AUD_OUT_BUFFER to DAC and reads sample from ADC to AUD_IN_BUFFER
static void AUD_IN_OUT() {
  if (!AUD_IN_BUFFER_FULL()) {
    int AUD_OUT_IDX = AUD_OUT_BUFFER_IDX + AUD_IN_BUFFER_IDX;
    dacWrite(AUD_OUT_PIN, AUD_OUT_BUFFER[0][AUD_OUT_IDX]);
    
    AUD_IN_BUFFER[AUD_IN_BUFFER_IDX] = adc1_get_raw(AUD_IN_PIN);
    AUD_IN_BUFFER_IDX += 1;
  }
}