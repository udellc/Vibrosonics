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
#define FFT_MAX_SUM 3000    // the maximum sum of FFT bins, this value is used when using breadslicer DET_MODE

#define FFT_AVG_WINDOW 8  // number of FFT windows to average using circular buffer

  // FFT data processing settings
#define DET_MODE 'p'  // 'p' = majorPeaks, 'b' = breadslicer
#define DET_DATA 'c'  // data to use by detection algorithm selected by DET_MODE, 'a' = FFTWindowsAvg, 'c' = FFTWindow
#define SCALING_MODE 'd'  // various frequency scaling modes 'a' through 'd', 'm' - mirror mode
                          // (Note: For true mirror mode, FREQ_MAX_AMP_DELTA/FREQ_MIN_AMP_DELTA must be 0, DET_MODE must be 'p' and DET_DATA must be 'c'!)

  // frequency of min and max amplitude change settings
#define FREQ_MAX_AMP_DELTA 1  // use frequency of max amplitude change function to weigh amplitude of most change

#define FREQ_MAX_AMP_DELTA_MIN 50   // the min threshold of change in amplitude to be considered significant by the frequencyMaxAmplitudeDelta() function
#define FREQ_MAX_AMP_DELTA_MAX 200  // the max threshold of change in amplitude
#define FREQ_MAX_AMP_DELTA_K 8.0    // weight for amplitude of most change

  // the maximum number of peaks to look for with the findMajorPeaks() function, also corresponds to how many sine waves are being synthesized
#define MAX_NUM_PEAKS 16

  // number of frequency bands, used for breadslicer() as well as mapping frequencies
#define NUM_FREQ_BANDS 5 // this must be equal to how many frequency bands the FFT spectrogram is being split into for the breadslicer and frequency mapping

const int freqBands[NUM_FREQ_BANDS] = { 60, 200, 500, 2000, int(SAMPLING_FREQ) >> 1 };  // stores the frequency bands (in Hz) associated to SUB_BASS through VIBRANCE

const float freqBandsK[NUM_FREQ_BANDS] = { 1.0, 1.0, 1.0, 1.0, 1.0 }; // additional weights for various frequency ranges used during amplitude scaling
const int freqBandsMapK[NUM_FREQ_BANDS] { 30, 45, 80, 100, 110 }; // used for mapping frequencies to explicitly defined frequencies

#define DEBUG 0    // in debug mode, the time taken in loop is printed (in microseconds), along with the assigned sine wave frequencies and amplitudes

/*/
########################################################
  Variables related to mode selection
########################################################
/*/

// used for configuring operation mode
void (*detectionFunc)(float*) = NULL;
float* detFuncData;

int* assignSinWavesFreq;
float* assignSinWavesAmp;
int assignSinWavesNum;

void (*weightingFunc)(int &, int &) = NULL;

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

float FFTWindows[int(FFT_AVG_WINDOW)][FFT_WINDOW_SIZE_BY2]; // stores FFT windows for averaging
float FFTWindowsAvg[FFT_WINDOW_SIZE_BY2];               // stores the averaged FFT windows
float FFTWindowsAvgPrev[FFT_WINDOW_SIZE_BY2];           // stores the previous averaged FFT windows
const float _FFT_AVG_WINDOW = 1.0 / FFT_AVG_WINDOW;
int averageWindowCount = 0;       // global variable for iterating through circular FFTWindows buffer

float FFTWindow[FFT_WINDOW_SIZE_BY2];   // stores the data from the current fft window
float FFTWindowPrev[FFT_WINDOW_SIZE_BY2]; // stores the data from the previous fft window

int FFTPeaks[FFT_WINDOW_SIZE_BY2 >> 1]; // stores the peaks computed by FFT
float FFTPeaksAmp[FFT_WINDOW_SIZE_BY2 >> 1];

int freqBandsIndexes[NUM_FREQ_BANDS]; // stores FFT bin indexes corresponding to freqBands array

// stores the peaks found by breadslicer and the sum of each band
int breadslicerPeaks[NUM_FREQ_BANDS];
float breadslicerSums[NUM_FREQ_BANDS];

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

const int MAX_NUM_WAVES = DET_MODE == 'b' ? NUM_FREQ_BANDS : MAX_NUM_PEAKS;
const int MAX_AMP_SUM = DET_MODE == 'b' ? FFT_MAX_SUM : MAX_NUM_WAVES * FFT_MAX_AMP;
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

// used to delay the audio output buffer by 2 FFT sampling periods so that audio can be generated on time, this results in a delay between input and output of 2 * FFT_WINDOW_SIZE / SAMPLING_FREQ
volatile int NUM_WINDOWS_REC = 0;

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

  // calculate indexes associated to frequency bands
  calculatefreqBands();

  // setup detection algorithm mode
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
      Serial.println("DETECTION FUNCTION UNATTACHED! USING MAJOR PEAKS");
      detectionFunc = &findMajorPeaks;
      assignSinWavesFreq = FFTPeaks;
      assignSinWavesAmp = FFTPeaksAmp;
      assignSinWavesNum = FFT_WINDOW_SIZE_BY2 >> 1;
      break;
  }

  // select data to use for detection algorithm, selected by DET_MODE
  switch(DET_DATA) {
    case 'a':
      detFuncData = FFTWindowsAvg;
      break;
    case 'c':
      detFuncData = FFTWindow;
      break;
    default:
      Serial.println("USING DEFAULT DETECTION DATA: FFTWindow");
      detFuncData = FFTWindow;
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
    default:
      Serial.println("USING MIRROR MODE!");
      weightingFunc = &ampFreqWeightingM;
      break;
  }

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
  RESET_AUD_IN_OUT_BUFFERS();


  FFT.DCRemoval();                                  // DC Removal to reduce noise
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
  FFT.Compute(FFT_FORWARD);                         // Compute FFT
  FFT.ComplexToMagnitude();                         // Compute frequency magnitudes

  // copy values calculated by FFT to FFTWindow and FFTWindows
  storeFFTWindow();
  
  // noise flooring based on mean of data and a threshold
  noiseFloor(detFuncData, 25.0);

  detectionFunc(detFuncData);

  // if @FREQ_MAX_AMP_DELTA was enabled, returns the index of the amplitude of most change within set threshold between FFT windows as well as the magnitude of change which is passed as a reference
  float maxAmpDeltaMag = 0.0;
  int maxAmpDeltaIdx = FREQ_MAX_AMP_DELTA ? frequencyMaxAmplitudeDelta(FFTWindowsAvg, FFTWindowsAvgPrev, maxAmpDeltaMag) : -1;
  
  int numSineWaves = assignSinWaves(assignSinWavesFreq, assignSinWavesAmp, assignSinWavesNum);

  // weight frequencies based on passed weighting function, returns the total sum of the amplitudes and the index of the max amplitude which is passed by reference
  int maxAmpIdx = 0;
  int totalEnergy = scaleAmplitudeFrequency(maxAmpIdx, maxAmpDeltaIdx, maxAmpDeltaMag, numSineWaves);

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
int assignSinWaves(int* freqData, float* ampData, int size) {
  // restore amplitudes to 0, restoring frequencies isn't necassary
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    sin_wave_amplitude[i] = 0;
    sin_wave_frequency[i] = 0;
  }
  int c = 0;
  // copy amplitudes that are above 0, otherwise skip
  for (int i = 0; i < size; i++) {
    // skip storing if amplitude idx or freqData is 0, since this corresponds to 0Hz
    if (ampData[i] > 0 && freqData[i] == 0) { continue; }
    sin_wave_amplitude[c] = round(ampData[i]);
    sin_wave_frequency[c++] = freqData[i];
    // break so no overflow occurs
    if (c == MAX_NUM_WAVES) { break; }
  }
  return c;
}

// iterates through sine waves and scales frequencies/amplitudes based on frequency bands and the frequency of max amplitude change
int scaleAmplitudeFrequency(int &maxAmpIdx, int maxAmpDeltaIdx, float maxAmpDeltaMag, int numWaves) {
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
    // use results computed by frequency of max and min amplitude change if enabled
    // if frequencyOfMaxAmplitudeChange() was enabled and found a amplitude change above threshold and it exists in the detected peak, then weigh this amplitude
    if (maxAmpDeltaIdx > -1 && frequency == maxAmpDeltaIdx) {
      amplitude = amplitude + round(amplitude * maxAmpDeltaMag);
    }

    // convert from index to frequency
    frequency = interpolateAroundPeak(detFuncData, frequency);

    // map frequencies and weigh amplitudes
    weightingFunc(frequency, amplitude);

    // reassign frequencies and amplitudes
    sin_wave_amplitude[i] = amplitude;
    sin_wave_frequency[i] = frequency;
    // sum the energy associated with the amplitudes
    sum += amplitude;
  }
  return sum;
}

// scales frequencies based on default values for frequency bands stored in freqBandsMapK, amplitude weighting based on freqBandsK
void ampFreqWeightingA(int &freq, int &amp) {
  for (int i = 0; i < NUM_FREQ_BANDS; i++) {
    if (freq <= freqBands[i]) {
      amp *= freqBandsK[i];
      freq = freqBandsMapK[i];
      break;
    }
  }
}

// scales frequencies based on values in between frequency bands stored in freqBandsMapK, amplitude weighting based on freqBandsK
void ampFreqWeightingB(int &freq, int &amp) {
  for (int i = 0; i < NUM_FREQ_BANDS; i++) {
    if (freq <= freqBands[i]) {
      amp *= freqBandsK[i];
      freq = map(freq, i > 0 ? freqBands[i - 1] : 1, freqBands[i], i > 0 ? freqBandsMapK[i - 1] : 25, freqBandsMapK[i]);
      break;
    }
  }
}

// scales frequency by mapping the full spectrum to the desired range
void ampFreqWeightingC(int &freq, int &amp) {
  freq = map(freq, 1, SAMPLING_FREQ * 0.5, 20, 120);
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
      amp *= freqBandsK[band];
      break;
    case 1:
      freq = round(freq * 0.25) + 25;
      amp *= freqBandsK[band];
      break;
    case 2:
      freq = round(freq * 0.125) + 20;
      amp *= freqBandsK[band];
      break;
    case 3:
      freq = round(freq * 0.0625) + 15;
      amp *= freqBandsK[band];
      break;
    default:
      freq = round(freq * 0.03125);
      amp *= freqBandsK[band];
      break;
  }
}

// mirror mode scaling, nothing happens to frequencies and amplitudes
void ampFreqWeightingM(int &freq, int &amp) {
  return;
}

// maps amplitudes between 0 and 127 range to correspond to 8-bit ADC on ESP32 Feather
void mapAmplitudes(int maxAmpIdx, int totalEnergy, int numWaves) {
  // weight of the max amplitude relative to other amplitudes
  float maxAmpWeight = sin_wave_amplitude[maxAmpIdx] / float(totalEnergy > 0 ? totalEnergy : 1.0);
  // if the weight of max amplitude is larger than a certain threshold, assume that a single frequency is being detected to decrease noise in the output
  int singleFrequency = maxAmpWeight > 0.9 ? 1 : 0;
  // value to map amplitudes between 0.0 and 1.0 range, the MAX_AMP_SUM will be used to divide unless totalEnergy exceeds this value
  float divideBy = 1.0 / float(totalEnergy > MAX_AMP_SUM ? totalEnergy : MAX_AMP_SUM);

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

// store the current window into FFTWindow and FFTWindows for averaging windows to reduce noise and produce a cleaner spectrogram for signal processing
void storeFFTWindow () {
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    float data = vReal[i] * freqWidth;
    // store oldFFTWindow into prevFFTWindow, and newly computed FFT results in FFTWindow
    FFTWindowPrev[i] = FFTWindow[i];
    FFTWindow[i] = data;

    // store data in FFTWindows array
    FFTWindows[averageWindowCount][i] = data;
    float amplitudeSum = 0.0;
    for (int j = 0; j < FFT_AVG_WINDOW; j++) {
      amplitudeSum += FFTWindows[j][i];
    }
    // store old averaged window in FFTWindowsAvgPrev, and the current averaged window in FFTWindowsAvg
    FFTWindowsAvgPrev[i] = FFTWindowsAvg[i];
    FFTWindowsAvg[i] = amplitudeSum * _FFT_AVG_WINDOW;

    //Serial.printf("idx: %d, averaged: %.2f, data: %.2f\n", i, vReal[i], FFTWindow[i]);
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

// noise flooring based on mean of data and threshold
void noiseFloorMean(float *data, float threshold) {
  float dataSum = 0.0;
  // get the mean of data
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    dataSum += data[i];
  }
  float dataMean = dataSum / float(FFT_WINDOW_SIZE_BY2);

  // subtract mean from data
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    data[i] -= dataMean;
    if (data[i] < threshold) { data[i] = 0.0; }
  }

}

// finds the frequency of most change within certain boundaries @FREQ_MAX_AMP_DELTA_MIN and @FREQ_MAX_AMP_DELTA_MAX
// returns the index of the frequency of most change, and stores the magnitude of change (between 0.0 and FREQ_MAX_AMP_DELTA_K) in changeMagnitude reference
int frequencyMaxAmplitudeDelta(float *data, float *prevData, float &changeMagnitude) {
  // restore global varialbes
  int maxAmpChange = 0;
  int maxAmpChangeIdx = -1;
  // iterate through data* and prevData* to find the amplitude with most change
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    // calculate the change between this amplitude and previous amplitude
    int currAmpChange = abs(int(data[i]) - int(prevData[i]));
    // find the most dominant amplitude change and store in maxAmpChangeIdx, store the magnitude of the change in maxAmpChange
    if ((currAmpChange >= FREQ_MAX_AMP_DELTA_MIN) && (currAmpChange <= FREQ_MAX_AMP_DELTA_MAX) && (currAmpChange > maxAmpChange)) {
      maxAmpChange = currAmpChange;
      maxAmpChangeIdx = i;
    }
  }
  changeMagnitude = maxAmpChange * (1.0 / FREQ_MAX_AMP_DELTA_MAX) * FREQ_MAX_AMP_DELTA_K;
  return maxAmpChangeIdx;
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

// slices fft data into bands, storing the maximum peak in the band and averaged sum of the total energy in the band
void breadslicer(float* data) {
  int sliceCount = 0;
  int sliceIdx = freqBandsIndexes[sliceCount];
  float sliceSum = 0.0;
  int numBins = freqBandsIndexes[sliceCount];
  int sliceMax = 0;
  int sliceMaxIdx = 0;
  for (int f = 0; f < FFT_WINDOW_SIZE_BY2; f++) {
    // break if on last index of data array
    if (f == FFT_WINDOW_SIZE_BY2 - 1) {
      breadslicerSums[sliceCount] = (sliceSum + data[f]);
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
      numBins = freqBandsIndexes[sliceCount] - freqBandsIndexes[sliceCount - 1];
      sliceIdx = freqBandsIndexes[sliceCount];
      f -= 1;
    }
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
void generateAudioForWindow(int *sin_waves_freq, int *sin_wave_amp, int num_sin_waves) {
  for (int i = 0; i < AUD_OUT_BUFFER_SIZE; i++) {
    // variable store the value of the sum of sine waves at this particular moment
    float sumOfSines = 0.0;
    // sum together the sine waves
    for (int s = 0; s < num_sin_waves; s++) {
      if (sin_wave_amp[s] == 0) { continue; }
      // calculate the sine wave index of the sine wave corresponding to the frequency
      //int sin_wave_position = (sin_wave_freq_idx) % int(SAMPLING_FREQ); // a slightly faster way of doing mod function to reduce usage of division
      float sin_wave_freq_idx = sin_wave_idx * sin_waves_freq[s] * _SAMPLING_FREQ;
      int sin_wave_position = (sin_wave_freq_idx - floor(sin_wave_freq_idx)) * SAMPLING_FREQ;
      sumOfSines += sin_wave_amp[s] * sin_wave[sin_wave_position];
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
void RESET_AUD_IN_OUT_BUFFERS() {
  // increment NUM_WINDOWS_REC to delay audio output buffer by 2 windows
  if (NUM_WINDOWS_REC < 2) { NUM_WINDOWS_REC += 1; }
  // reset volatile audio input buffer position
  AUD_IN_BUFFER_IDX = 0;
  // reset volatile audio output buffer position, if needed
  if (AUD_OUT_BUFFER_IDX < AUD_OUT_BUFFER_SIZE) { return; }
  AUD_OUT_BUFFER_IDX = 0;
}

// outputs a sample from the volatile output buffer to DAC
void OUTPUT_SAMPLE() {
  // delay audio output by 2 windows
  if (NUM_WINDOWS_REC < 2) { return; }
  dacWrite(AUDIO_OUT_PIN, AUD_OUT_BUFFER[AUD_OUT_BUFFER_IDX++]);
}

// records a sample from the ADC and stores into volatile input buffer
void RECORD_SAMPLE() {
  AUD_IN_BUFFER[AUD_IN_BUFFER_IDX++] = adc1_get_raw(AUDIO_IN_PIN);
}
