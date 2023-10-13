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
#define AUDIO_INPUT_PIN ADC1_CHANNEL_6 // corresponds to ADC on A2
#define AUDIO_OUTPUT_PIN A0

#define FFT_WINDOW_SIZE 256   // Number of FFT windows, this value must be a power of 2
#define SAMPLING_FREQ 10000   // The audio sampling and audio output rate, with the ESP32 the fastest we could sample at is just under 24kHz, but
                              // bringing this down leaves more processing time for FFT, signal processing functions, and synthesis

#define FFT_MAX_SUM 3000    // the maximum sum of amplitudes of FFT, used for volume representation

#define FFT_AVG_WINDOW 4  // number of FFT windows to average using circular buffer

  // FFT data processing settings
#define MIRROR_MODE 0 // when enabled, frequencies and amplitudes calculated by FFT are directly mapped without any scaling or weighting to represent the "raw" signal
#define DET_MODE 'p' // 'p' = majorPeaks, 'b' = breadslicer
#define SCALING_MODE 'a' // frequency scaling mode 'a' through 'e'

#define FREQ_MAX_AMP_DELTA 1  // use frequency of max amplitude change function to weigh amplitude of most change
#define FREQ_MIN_AMP_DELTA 1  // use frequency of min amplitude change function to weigh amplitude of least change

#define FREQ_MAX_AMP_DELTA_MIN 25   // the min threshold of change in amplitude to be considered significant by the frequencyMaxAmplitudeDelta() function
#define FREQ_MAX_AMP_DELTA_MAX 150  // the max threshold of change in amplitude
#define FREQ_MAX_AMP_DELTA_K 32.0    // weight for amplitude of most change

#define FREQ_MIN_AMP_DELTA_MIN 20

#define MAX_NUM_PEAKS 16  // the maximum number of peaks to look for with the findMajorPeaks() function, also corresponds to how many sine waves are being synthesized

#define NUM_FREQ_BANDS 6 // this must be equal to how many frequency bands the FFT spectrogram is being split into for the breadslicer

const int freqBands[NUM_FREQ_BANDS] = { 60, 200, 500, 2000, 4000, int(SAMPLING_FREQ) >> 1 };  // stores the frequency bands (in Hz) associated to SUB_BASS through VIBRANCE

const float freqBandsK[NUM_FREQ_BANDS] = { 8.0, 1.0, 2.0, 3.0, 4.0, 1.0 }; // additional weights for various frequency ranges used during amplitude scaling
const int freqBandsMapK[NUM_FREQ_BANDS] { 32, 44, 60, 74, 90, 110 }; // used for mapping frequencies to explicitly defined frequencies

#define DEBUG 0

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

float FFTWindows[int(FFT_AVG_WINDOW)][FFT_WINDOW_SIZE_BY2]; // stores FFT windows for averaging
float FFTWindowsAvg[FFT_WINDOW_SIZE_BY2];               // stores the averaged FFT windows
float FFTWindowsAvgPrev[FFT_WINDOW_SIZE_BY2];           // stores the previous averaged FFT windows
const float _FFT_AVG_WINDOW = 1.0 / FFT_AVG_WINDOW;
int averageWindowCount = 0;       // global variable for iterating through circular FFTWindows buffer

float FFTData[FFT_WINDOW_SIZE_BY2];   // stores the data the signal processing functions will use
float FFTDataPrev[FFT_WINDOW_SIZE_BY2]; // stores the previous data used for analysis to find frequency of max amplitude change between consecutive windows

int FFTPeaks[FFT_WINDOW_SIZE_BY2 >> 1]; // stores the peaks computed by FFT
int FFTPeaksAmp[FFT_WINDOW_SIZE_BY2 >> 1];

// stores the peaks found by breadslicer and the sum of each band
int freqBandsIndexes[NUM_FREQ_BANDS]; // stores FFT bin indexes corresponding to freqBands array
int breadslicerPeaks[NUM_FREQ_BANDS];
int breadslicerSums[NUM_FREQ_BANDS];

// used for configuring operation mode
void (*detectionFunc)(float*) = NULL;
float* detFuncData;

int* assignSinWavesFreq;
int* assignSinWavesAmp;
int assignSinWavesNum;

void (*weightingFunc)(int &, int &) = NULL;

/*/
########################################################
  Variables used for generating an audio output
########################################################
/*/

const int AUDIO_INPUT_BUFFER_SIZE = int(FFT_WINDOW_SIZE);
const int AUDIO_OUTPUT_BUFFER_SIZE = int(FFT_WINDOW_SIZE) * 3;
const int FFT_WINDOW_SIZE_X2 = FFT_WINDOW_SIZE * 2;

const float _SAMPLING_FREQ = 1.0 / SAMPLING_FREQ;

// a cosine wave for modulating sine waves
float cos_wave[FFT_WINDOW_SIZE_X2];

// sine wave for storing pre-generated values of a sine wave at SAMPLING_FREQ sample rate
float sin_wave[SAMPLING_FREQ];

const int MAX_NUM_WAVES = NUM_FREQ_BANDS > MAX_NUM_PEAKS ? NUM_FREQ_BANDS : MAX_NUM_PEAKS;
// stores the sine wave frequencies and amplitudes to synthesize
int sin_wave_frequency[MAX_NUM_WAVES];
int sin_wave_amplitude[MAX_NUM_WAVES];

// stores the position of sin_wave
int sin_wave_idx = 0;

// scratchpad array used for signal synthesis
float audioOutputBuffer[AUDIO_OUTPUT_BUFFER_SIZE];
// stores the current index of the scratch pad audio output buffer
int generateAudioIdx = 0;
// used for copying final synthesized values from scratchpad audio output buffer to volatile audio output buffer
int rollingAudioOutputBufferIdx = 0;

/*/
########################################################
  Variables used for interrupt and function to call when interrupt is triggered
########################################################
/*/

// used to store recorded samples
volatile int audioInputBuffer[AUDIO_INPUT_BUFFER_SIZE];
volatile int audioInputBufferIdx = 0;
volatile int audioInputBufferFull = 0;

// rolling buffer for outputting synthesized signal
volatile int audioOutputBufferVolatile[FFT_WINDOW_SIZE_X2];
volatile int audioOutputBufferIdx = 0;

// used to delay the audio output buffer by 2 FFT sampling periods so that audio can be generated on time, this results in a delay between input and output of 2 * FFT_WINDOW_SIZE / SAMPLING_FREQ
volatile int numFFTCount = 0;

hw_timer_t *SAMPLING_TIMER = NULL;

// the procedure that is called when timer interrupt is triggered
void IRAM_ATTR onTimer(){
  outputSample();
  recordSample();
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

  // calculate indexes associated to frequency bands
  calculatefreqBands();
  
  //detFuncData = FFTWindowsAvg;
  detFuncData = FFTData;

  // attach detection algorithm
  switch(DET_MODE) {
    case 'p':
      detectionFunc = &findMajorPeaks;
      assignSinWavesFreq = FFTPeaks;
      assignSinWavesAmp = FFTPeaksAmp;
      assignSinWavesNum = FFT_WINDOW_SIZE_BY2 >> 1;
      break;
    case 'b':
      detectionFunc = &breadslicer;
      assignSinWavesFreq = breadslicerPeaks;
      assignSinWavesAmp = breadslicerSums;
      assignSinWavesNum = NUM_FREQ_BANDS;
      break;
    default:
      Serial.println("DETECTION FUNCTION UNATTACHED!");
      break;
  }

  // attach frequency/amplitude scaling function
  switch(SCALING_MODE) {
    case 'a':
      weightingFunc = &ampFreqWeightingA;
      break;
    case 'b':
      weightingFunc = &ampFreqWeightingB;
      break;
    case 'c':
      weightingFunc = &ampFreqWeightingC;
      break;
    case 'd':
      weightingFunc = &ampFreqWeightingD;
      break;
    case 'e':
      weightingFunc = &ampFreqWeightingE;
      break;
    default:
      Serial.println("WEIGHTING FUNCTION UNATTACHED!");
      break;
  }

  // setup pins
  pinMode(AUDIO_OUTPUT_PIN, OUTPUT);

  Serial.println("Setup complete");

  delay(1000);

  // setup timer interrupt for audio sampling
  SAMPLING_TIMER = timerBegin(0, 80, true);                 // setting clock prescaler 1MHz (80MHz / 80)
  timerAttachInterrupt(SAMPLING_TIMER, &onTimer, true);     // attach interrupt function
  timerAlarmWrite(SAMPLING_TIMER, sampleDelayTime, true);   // trigger interrupt every @sampleDelayTime microseconds
  timerAlarmEnable(SAMPLING_TIMER);     //Just Enable
  timerStart(SAMPLING_TIMER);
}

/*/
########################################################
  Loop
########################################################
/*/

void loop() {
  if (audioInputBufferFull == 1) {
    //timerAlarmDisable(My_timer);
    //unsigned long time = micros();

    // fft data processing, returns num sine waves to synthesize
    int numSineWaves = processData();

    // generate audio for the next audio window
    generateAudioForWindow(sin_wave_frequency, sin_wave_amplitude, numSineWaves);

    //Serial.println(micros() - time);
    //timerAlarmEnable(My_timer);
  }
}

int processData() {
  // copy values from audioInputBuffer to vReal array
  setupFFT();
  
  FFT.DCRemoval();                                  // DC Removal to reduce noise
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
  FFT.Compute(FFT_FORWARD);                         // Compute FFT
  FFT.ComplexToMagnitude();                         // Compute frequency magnitudes

  // copy values calculated by FFT to FFTData and FFTWindows
  storeFFTData();
  
  // noise flooring based on hreshold
  noiseFloor(FFTData, 25.0);
  noiseFloor(FFTWindowsAvg, 15.0);
  
  // if @FREQ_MAX_AMP_DELTA was enabled, returns the index of the amplitude of most change within set threshold between consecutive FFT windows as well as the magnitude of change which is passed as a reference
  float maxAmpDeltaMag = 0.0;
  int maxAmpDeltaIdx = FREQ_MAX_AMP_DELTA ? frequencyMaxAmplitudeDelta(FFTData, FFTDataPrev, maxAmpDeltaMag) : -1;

  float minAmpDeltaMag = 1.0;
  int minAmpDeltaIdx = FREQ_MIN_AMP_DELTA ? frequencyMinAmplitudeDelta(FFTWindowsAvg, FFTWindowsAvgPrev, minAmpDeltaMag) : -1;

  detectionFunc(detFuncData);
  int numSineWaves = assignSinWaves(assignSinWavesFreq, assignSinWavesAmp, assignSinWavesNum);

  // weight frequencies based on passed weighting function, returns the total sum of the amplitudes and the index of the max amplitude which is passed by reference
  int maxAmpIdx = 0;
  int totalEnergy = scaleAmplitudeFrequency(maxAmpIdx, maxAmpDeltaIdx, maxAmpDeltaMag, minAmpDeltaIdx, minAmpDeltaMag, numSineWaves);

  // maps the amplitudes so that their total sum is no more or equal to 127
  mapAmplitudes(maxAmpIdx, totalEnergy, numSineWaves);

  return numSineWaves;
}

/*/
########################################################
  Functions relating to assigning sine waves and mapping their frequencies and amplitudes
########################################################
/*/

// stores the frequencies and amplitudes found by majorPeaks into separate arrays, and returns the number of sin waves to synthesize
int assignSinWaves(int* ampIdx, int* ampData, int size) {
  // restore amplitudes to 0, restoring frequencies isn't necassary
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    sin_wave_amplitude[i] = 0;
  }
  int c = 0;
  // copy amplitudes that are above 0, otherwise skip
  for (int i = 0; i < size; i++) {
    int amplitudeIdx = ampIdx[i];
    // skip storing if amplitude idx is 0, since this corresponds to 0Hz
    if (amplitudeIdx == 0) { continue; }
    sin_wave_amplitude[c] = ampData[i];
    sin_wave_frequency[c++] = amplitudeIdx;
    // break so no overflow occurs
    if (c == MAX_NUM_WAVES) { break; }
  }
  return c;
}

// iterates through sine waves and scales frequencies/amplitudes based on frequency bands and the frequency of max amplitude change
int scaleAmplitudeFrequency(int &maxAmpIdx, int maxAmpDeltaIdx, float maxAmpDeltaMag, int minAmpDeltaIdx, float minAmpDeltaMag, int numWaves) {
  int sum = 0;
  int maxAmp = 0;
  // iterate through sin waves
  for (int i = 0; i < numWaves; i++) {
    int amplitude = sin_wave_amplitude[i];
    int frequency = sin_wave_frequency[i];

    // find max amplitude
    if (amplitude > maxAmp) {
      maxAmp = amplitude;
      maxAmpIdx = i;
    }

    // if mirror mode is enabled then skip frequency scaling and amplitude weighting to represent the raw input signal
    if (!MIRROR_MODE) {
      // use results computed by frequencyOfMaxAmplitudeChange if enabled
      // if frequencyOfMaxAmplitudeChange() was enabled and found a amplitude change above threshold and it exists in the detected peak, then weigh this amplitude
      if (maxAmpDeltaIdx > -1 && frequency == maxAmpDeltaIdx) {
        amplitude = round(amplitude * maxAmpDeltaMag);
      }
      if (minAmpDeltaIdx > -1 && frequency == minAmpDeltaIdx) {
        amplitude = round(amplitude * minAmpDeltaMag);
      }
      frequency = interpolateAroundPeak(detFuncData, frequency);
      // map frequencies and weigh amplitudes
      weightingFunc(frequency, amplitude);
    } else {
      // convert from index to frequency
      frequency = interpolateAroundPeak(detFuncData, frequency);
    }
    // reassign frequencies and amplitudes
    sin_wave_amplitude[i] = amplitude;
    sin_wave_frequency[i] = frequency;
    // sum the energy associated with the amplitudes
    sum += amplitude;
  }
  return sum;
}

// scales frequencies based on default values for frequency bands stored in freqBandsMapK, leave amplitudes the same
void ampFreqWeightingA(int &freq, int &amp) {
  for (int i = 0; i < NUM_FREQ_BANDS; i++) {
    if (freq <= freqBands[i]) {
      amp *= freqBandsK[i];
      freq = freqBandsMapK[i];
      break;
    }
  }
}

// scales frequencies by dividing each band by a consective power of 2, leave amplitudes the same
void ampFreqWeightingB(int &freq, int &amp) {
  // amp *= a_weightingFilter(freq);
  for (int i = 0; i < NUM_FREQ_BANDS; i++) {
    if (freq <= freqBands[i]) {
      freq = freq >> i;
      break;
    }
  }
}

// scales frequencies past 200Hz
void ampFreqWeightingC(int &freq, int &amp) {
  if (freq > 200) {
    freq = map(freq, 200, SAMPLING_FREQ / 2, 40, 120);
  }
}

// custom frequency scaling and amplitude weighting
void ampFreqWeightingD(int &freq, int &amp) {
  int band;
  for (int i = 0; i < NUM_FREQ_BANDS; i++) {
    if (freq <= freqBands[i]) {
      band = i;
      break;
    }
  }
  switch (band) {
    case 0:
      freq = freq;
      break;
    case 1:
      freq = freq * 0.25;
      break;
    case 2:
      freq = round(pow(freq, 0.77));
      break;
    case 3:
      freq = round(pow(freq, 0.66));
      break;
    case 4:
      freq = round(pow(freq, 0.62));
      break;
    case 5:
      freq = round(pow(freq, 0.6));
      break;
  }
}

// custom frequency scaling and amplitude weighting
void ampFreqWeightingE(int &freq, int &amp) {
  int band;
  for (int i = 0; i < NUM_FREQ_BANDS; i++) {
    if (freq <= freqBands[i]) {
      band = i;
      break;
    }
  }
  switch (band) {
    case 0:
      freq = freq;
      break;
    case 1:
      freq = freq;
      break;
    case 2:
      freq = pow(round(freq * 0.25), 1.05);
      break;
    case 3:
      freq = pow(round(freq * 0.125), 0.96);
      break;
    case 4:
      freq = pow(round(freq * 0.0625), 0.96);
      break;
    case 5:
      freq = pow(round(freq * 0.03125), 1.04); 
      break;
  }
}

// maps amplitudes between 0 and 127 range to correspond to 8-bit ADC on ESP32 Feather
void mapAmplitudes(int maxAmpIdx, int totalEnergy, int numWaves) {
  // weight of the max amplitude relative to other amplitudes
  float maxAmpWeight = sin_wave_amplitude[maxAmpIdx] / float(totalEnergy > 0 ? totalEnergy : 1.0);
  // if the weight of max amplitude is larger than a certain threshold, assume that a single frequency is being detected to decrease noise in the output
  int singleFrequency = maxAmpWeight > 0.9 ? 1 : 0;
  // value to map amplitudes between 0.0 and 1.0 range, the FFT_MAX_SUM will be used to divide unless totalEnergy exceeds this value
  float divideBy = 1.0 / float(totalEnergy > FFT_MAX_SUM ? totalEnergy : FFT_MAX_SUM);

  // normalizing and multiplying to ensure that the sum of amplitudes is less than or equal to 127
  for (int i = 0; i < numWaves; i++) {
    int amplitude = sin_wave_amplitude[i];
    if (amplitude == 0) { continue; }
    // map amplitude between 0 and 127
    int mappedAmplitude = floor(amplitude * divideBy * 127.0);
    // if assuming single frequency is detected based on the weight of the max amplitude,
    if (singleFrequency) {
      // assign other amplitudes to 0 to reduce noise
      sin_wave_amplitude[i] = i == maxAmpIdx ? mappedAmplitude : 0;
    } else {
      // otherwise assign normalized amplitudes
      sin_wave_amplitude[i] = mappedAmplitude;
    }
    //Serial.printf("(%03d %03d)\t", sin_wave_amplitude[i], sin_wave_frequency[i]);
  }
  //Serial.println();
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
    vReal[i] = audioInputBuffer[i];
    // set imaginary values to 0
    vImag[i] = 0.0;
  }
  // reset flag
  audioInputBufferFull = 0;
}

// store the current window into FFTData and FFTWindows for averaging windows to reduce noise and produce a cleaner spectrogram for signal processing
void storeFFTData () {
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    float data = vReal[i] * freqWidth;
    // store current window in FFTData
    FFTData[i] = data;

    // store data in FFTWindows array
    FFTWindows[averageWindowCount][i] = data;
    float amplitudeSum = 0.0;
    for (int j = 0; j < FFT_AVG_WINDOW; j++) {
      amplitudeSum += FFTWindows[j][i];
    }
    // store averaged window
    FFTWindowsAvg[i] = amplitudeSum * _FFT_AVG_WINDOW;

    //Serial.printf("idx: %d, averaged: %.2f, data: %.2f\n", i, vReal[i], FFTData[i]);
  }
  averageWindowCount += 1;
  if (averageWindowCount == FFT_AVG_WINDOW) { averageWindowCount = 0; }
}

// noise flooring based on a set threshold
void noiseFloor(float *data, float threshold) {
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    if (data[i] < threshold) {
      data[i] = 0.0;
    }
  }
}

// finds the frequency of most change within certain boundaries @FREQ_MAX_AMP_DELTA_MIN and @FREQ_MAX_AMP_DELTA_MAX
// returns the index of the frequency of most change, and stores the magnitude of change (between 0.0 and FREQ_MAX_AMP_DELTA_K) in changeMagnitude reference
int frequencyMaxAmplitudeDelta(float *data, float *prevData, float &changeMagnitude) {
  // restore global varialbes
  int maxAmpChange = 0;
  int maxAmpChangeIdx = -1;
  // iterate through data* and prevData* to find the amplitude with most change
  for (int i = 1; i < FFT_WINDOW_SIZE_BY2; i++) {
    // store the change of between this amplitude and previous amplitude
    int currAmpChange = abs(int(data[i]) - int(prevData[i]));
    // find the most dominant amplitude change and store in maxAmpChangeIdx, store the magnitude of the change in maxAmpChange
    if ((currAmpChange >= FREQ_MAX_AMP_DELTA_MIN) && (currAmpChange <= FREQ_MAX_AMP_DELTA_MAX) && (currAmpChange > maxAmpChange)) {
      maxAmpChange = currAmpChange;
      maxAmpChangeIdx = i;
    }
    // assign data to previous data
    prevData[i] = data[i];
  }
  changeMagnitude = maxAmpChange * (1.0 / FREQ_MAX_AMP_DELTA_MAX) * FREQ_MAX_AMP_DELTA_K;
  return maxAmpChangeIdx;
}

// finds the frequency of least change within @FREQ_MIN_AMP_DELTA_MIN
// returns the index of the frequency of least change, and stores the magnitude of change (between 0.0 and 1.0) in changeMagnitude reference
int frequencyMinAmplitudeDelta(float *data, float *prevData, float &changeMagnitude) {
  // restore global varialbes
  int minAmpChange = 0;
  int minAmpChangeIdx = -1;
  // iterate through data* and prevData* to find the amplitude with most change
  for (int i = 1; i < FFT_WINDOW_SIZE_BY2; i++) {
    // store the change of between this amplitude and previous amplitude
    if (data[i] > 0 && prevData[i] > 0) {
      int currAmpChange = abs(int(data[i]) - int(prevData[i]));
      // find the most dominant amplitude change and store in maxAmpChangeIdx, store the magnitude of the change in maxAmpChange
      if ((currAmpChange < FREQ_MIN_AMP_DELTA_MIN) && (currAmpChange < minAmpChange)) {
        minAmpChange = currAmpChange;
        minAmpChangeIdx = i;
      }
    }
    // assign current data to previous data
    prevData[i] = data[i];
  }
  changeMagnitude = minAmpChange * (1.0 / FREQ_MIN_AMP_DELTA_MIN);
  return minAmpChangeIdx;
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

// looks for harmonics associated with a freqIdx in data*. Set @replace to 1 to replace harmonics amplitudes to 0. Set output to 1 to output number of harmonics found, set to 0 to output total combined energy of harmonics
int findHarmonics(int freqIdx, float* data, int maxNumHarmonics, bool replace, bool output) {
  // return if freq is 0Hz
  int f = freqIdx;
  if (f = 0) { return 0; }
  float totalEnergy = data[f - 1] + data[f] + data[f + 1];
  int numHarmonicsFound = 0;
  int harmonicsCounter = 2;
  while (f < FFT_WINDOW_SIZE_BY2 - 1) {
    f = freqIdx * harmonicsCounter;
    bool harmonicFound = 0;
    if ((data[f - 1] < data[f]) && (data[f] > data[f + 1])) {
      totalEnergy += data[f - 1] + data[f] + data[f + 1];
      harmonicFound = 1;
    }
    // break if no peak was found near next harmonic
    if (!harmonicFound || numHarmonicsFound == maxNumHarmonics) { break; }
    else {
      numHarmonicsFound += 1;
      harmonicsCounter += 1;
      if (replace) {
        data[f - 1] = 0;
        data[f] = 0;
        data[f + 1] = 0;
      }
    }
  }
  return output == 1 ? numHarmonicsFound : totalEnergy;
}

// calculates indexes corresponding to frequency bands
void calculatefreqBands() {
  // converting from frequency to index
  for (int i = 0; i < NUM_FREQ_BANDS; i++) {
    freqBandsIndexes[i] = ceil(freqBands[i] * freqWidth);
  }
}

// slices fft data into bands, storing the maximum peak in the band and sum of the total energy in the band
void breadslicer(float* data) {
  int sliceCount = 0;
  int sliceIdx = freqBandsIndexes[sliceCount];
  int sliceSum = 0;
  int sliceMax = 0;
  int sliceMaxIdx = 0;
  for (int f = 0; f < FFT_WINDOW_SIZE_BY2; f++) {
    // break if on last index of data array
    if (f == FFT_WINDOW_SIZE_BY2 - 1) {
      breadslicerSums[sliceCount] = sliceSum + data[f];
      breadslicerPeaks[sliceCount] = sliceMaxIdx;
      break; 
    }
    // get the sum and max amplitude of the current slice
    if (f < sliceIdx) {
      if (data[f] == 0) { continue; }
      sliceSum += data[f];
      if (f == 0) { continue; }
      // look for maximum peak in slice
      if ((data[f - 1] < data[f]) && (data[f] > data[f + 1])) {
        // compare with previous max amplitude at peak
        if (data[f] > sliceMax) {
          sliceMax = data[f];
          sliceMaxIdx = f;
        }
      }
    // if exceeding the index of this slice, store sliceSum and sliceMaxIdx
    } else {
      breadslicerSums[sliceCount] = sliceSum;       // store slice sum
      breadslicerPeaks[sliceCount] = sliceMaxIdx;   // store slice peak
      sliceSum = 0;     // reset variables
      sliceMax = 0;
      sliceMaxIdx = 0;
      sliceCount += 1;  // iterate slice
      // break if sliceCount exceeds NUM_FREQ_BANDS
      if (sliceCount == NUM_FREQ_BANDS) { break; }
      sliceIdx = freqBandsIndexes[sliceCount];
      f -= 1;
    }
  }
}

// interpolation based on the weight of amplitudes around a peak
int interpolateAroundPeak(float *data, int indexOfPeak) {
  float prePeak = indexOfPeak == 0 ? 0 : data[indexOfPeak - 1];
  float atPeak = data[indexOfPeak];
  float postPeak = indexOfPeak == FFT_WINDOW_SIZE_BY2 ? 0 : data[indexOfPeak + 1];
  // summing around the index of maximum amplitude to normalize magnitudeOfChange
  float peakSum = prePeak + atPeak + postPeak;
  // interpolating the direction and magnitude of change, and normalizing from -1.0 to 1.0
  float magnitudeOfChange = ((atPeak + postPeak) - (atPeak + prePeak)) / peakSum;
  
  // return interpolated frequency
  return round((float(indexOfPeak) + magnitudeOfChange) * freqRes);
}

/*/
########################################################
  Functions related to generating values for the circular audio output buffer
########################################################
/*/

// calculate values for cosine function that is used for smoothing transition between frequencies (0.5 * (1 - cos((2PI / T) * x)), where T = FFT_WINDOW_SIZE_X2
void calculateCosWave() {
  float resolution = float(2.0 * PI / FFT_WINDOW_SIZE_X2);
  for (int i = 0; i < FFT_WINDOW_SIZE_X2; i++) {
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
void generateAudioForWindow(int *sin_waves_freq, int *sin_wave_amp, int num_sin_waves) {
  for (int i = 0; i < FFT_WINDOW_SIZE_X2; i++) {
    // variable store the value of the sum of sine waves at this particular moment
    float sumOfSines = 0.0;
    // sum together the sine waves
    for (int s = 0; s < num_sin_waves; s++) {
      if (sin_wave_amp[s] == 0) { continue; }
      // calculate the sine wave index of the sine wave corresponding to the frequency
      //int sin_wave_position = (sin_wave_freq_idx) % int(SAMPLING_FREQ);
      // a faster way of doing mod function to reduce usage of division
      float sin_wave_freq_idx = sin_wave_idx * sin_waves_freq[s] * _SAMPLING_FREQ;
      int sin_wave_position = (sin_wave_freq_idx - floor(sin_wave_freq_idx)) * SAMPLING_FREQ;
      sumOfSines += sin_wave_amp[s] * sin_wave[sin_wave_position];
    }
    // window sum of sines by cos wave at this moment in time
    float synthesized_value = cos_wave[i] * sumOfSines;
    // add to the existing values in scratch pad audio output buffer
    audioOutputBuffer[generateAudioIdx] += synthesized_value;

    // copy final, synthesized values to volatile audio output buffer
    if (i < FFT_WINDOW_SIZE) {
      audioOutputBufferVolatile[rollingAudioOutputBufferIdx++] = round(audioOutputBuffer[generateAudioIdx] + 128.0);
      if (rollingAudioOutputBufferIdx == FFT_WINDOW_SIZE_X2) { rollingAudioOutputBufferIdx = 0; }
    }

    // increment generate audio index
    generateAudioIdx += 1;
    if (generateAudioIdx == AUDIO_OUTPUT_BUFFER_SIZE) { generateAudioIdx = 0; }
    // increment sine wave index
    sin_wave_idx += 1;
    if (sin_wave_idx == SAMPLING_FREQ) { sin_wave_idx = 0; }
  }

  // reset the next window to synthesize new signal
  int generateAudioIdxCpy = generateAudioIdx;
  for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
    audioOutputBuffer[generateAudioIdxCpy++] = 0.0;
    if (generateAudioIdxCpy == AUDIO_OUTPUT_BUFFER_SIZE) { generateAudioIdxCpy = 0; }
  }
  // determine the next position in the scratch pad audio output buffer to counter phase cosine wave
  generateAudioIdx = int(generateAudioIdx - FFT_WINDOW_SIZE + AUDIO_OUTPUT_BUFFER_SIZE) % int(AUDIO_OUTPUT_BUFFER_SIZE);
  sin_wave_idx = int(sin_wave_idx - FFT_WINDOW_SIZE + SAMPLING_FREQ) % int(SAMPLING_FREQ);
}

/*/
########################################################
  Functions that get called when interrupt is triggered, audio input and output
########################################################
/*/

// outputs a sample from the volatile output buffer
void outputSample() {
  // delay audio output by 2 windows
  if (numFFTCount > 1) {
    dacWrite(AUDIO_OUTPUT_PIN, audioOutputBufferVolatile[audioOutputBufferIdx++]);
    if (audioOutputBufferIdx == FFT_WINDOW_SIZE_X2) {
      audioOutputBufferIdx = 0;
    }
  }
}

// records a sample from the ADC and stores into volatile input buffer
void recordSample() {
  audioInputBuffer[audioInputBufferIdx++] = adc1_get_raw(AUDIO_INPUT_PIN);
  if (audioInputBufferIdx == AUDIO_INPUT_BUFFER_SIZE) {
    if (numFFTCount < 2) { numFFTCount++; }
    audioInputBufferFull = 1;
    audioInputBufferIdx = 0;
  }
}
