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
#define AUD_IN_PIN ADC1_CHANNEL_6  // corresponds to ADC on A2
#define AUD_OUT_PIN A0

#define FFT_WINDOW_SIZE 256  // size of a recording window, this value MUST be a power of 2
#define SAMPLING_FREQ 10000  // The audio sampling and audio output rate, with the ESP32 the fastest we could sample at is just under 24kHz, but \
                             // bringing this down leaves more processing time for FFT, signal processing functions, and synthesis

#define FFT_NOISE_THRESH 25

#define FFT_MAX_AMP 400   // the maximum frequency magnitude of an FFT bin, this is multiplied by the total number of waves being synthesized for volume representation
#define FFT_MAX_SUM 4000  // the maximum sum of FFT bins, this value is used when using breadslicer DET_MODE

#define AUD_OUT_CH 1  // number of audio channels to synthesize

#define FFT_WINDOW_COUNT 8  // number of FFT windows to average using circular buffer

// FFT data processing settings
#define DET_MODE 'p'      // 'p' = majorPeaks, 'b' = breadslicer
#define DET_DATA 'c'      // data to use by detection algorithm selected by DET_MODE, 'a' = rawFreqsAvg, 'c' = freqs
#define SCALING_MODE 'c'  // various frequency scaling modes 'a' through 'd', 'm' - mirror mode
                          // (Note: For true mirror mode, FREQ_MAX_AMP_DELTA must be 0, DET_MODE must be 'p' and DET_DATA must be 'c'!)
                          //   'a' - directly maps passed frequency to values stored in freqBandsMapK, and weights amplitude based on values stored in freqBandsK.
                          //   'b' - maps passed frequencies to values between freqBandsMapK[i - 1] and freqBandsMapK[i] where i is the frequency band associated with the passed frequency. Amplitude weighting is based on the values stored in freqBandsK.
                          //   'c' - maps the full frequency spectrum (1Hz to (SAMPLING_FREQ / 2) Hz) to range between 20 and 120 Hz.
                          //   'd' - frequency and amplitude get scaled based on custom values.
                          //   'm' - mirror mode, nothing happens to frequencies and amplitudes.

// frequency of min and max amplitude change settings
#define FREQ_MAX_AMP_DELTA 1  // use frequency of max amplitude change function to weigh amplitude of most change

#define FREQ_MAX_AMP_DELTA_MIN 20     // the min threshold of change in amplitude to be considered significant by the frequencyMaxAmplitudeDelta() function
#define FREQ_MAX_AMP_DELTA_MAX 200    // the max threshold of change in amplitude
#define FREQ_MAX_AMP_DELTA_START 100  // the minimum frequency to look for max amplitude change
#define FREQ_MAX_AMP_DELTA_END 4000   // the maximum frequency to look for max amplitude change
#define FREQ_MAX_AMP_DELTA_K 16.0      // weight for amplitude of most change

// the maximum number of peaks to look for with the findMajorPeaks() function, also corresponds to how many sine waves are being synthesized
#define MAX_NUM_PEAKS 8

// number of frequency bands, used for breadslicer() as well as mapping frequencies
#define NUM_FREQ_BANDS 5  // this must be equal to how many frequency bands the FFT spectrogram is being split into for the breadslicer and frequency mapping

const int freqBands[NUM_FREQ_BANDS] = { 60, 200, 500, 2000, int(SAMPLING_FREQ) >> 1 };  // stores the frequency bands (in Hz) associated to SUB_BASS through VIBRANCE

const float freqBandsK[NUM_FREQ_BANDS] = { 1.5, 1.0, 1.5, 1.25, 2.0 };  // additional weights for various frequency ranges used during amplitude scaling
const int freqBandsMapK[NUM_FREQ_BANDS]{ 30, 48, 80, 100, 110 };        // used for mapping frequencies to explicitly defined frequencies

// #define DEBUG   // uncomment for debug mode, the time taken in loop is printed (in microseconds), along with the assigned sine wave frequencies and amplitudes

/*/
########################################################
  Variables related to mode selection
########################################################
/*/

// used for configuring operation mode
void (*detectionFunc)(float *) = NULL;
float *detFuncData;

int *assignSinWavesFreq;
float *assignSinWavesAmp;
int assignSinWavesNum;

void (*weightingFunc)(int &, int &) = NULL;

/*/
########################################################
  Other constants
########################################################
/*/

const float a_weighting_k1 = pow(107.7 * 0.1, 2);
const float a_weighting_k2 = pow(737.9 * 0.1, 2);

const float b_weighting_k1 = pow(158.5 * 0.1, 2);

const float c_weighting_k1 = pow(12200 * 0.1, 2);
const float c_weighting_k2 = pow(20.6 * 0.1, 2);

/*/
########################################################
  FFT
########################################################
/*/

const int sampleDelayTime = int(1000000 / SAMPLING_FREQ);  // the time delay between sample

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

const float freqWidth = 1.0 / freqRes;  // the width of an FFT bin

float rawFreqs[int(FFT_WINDOW_COUNT)][FFT_WINDOW_SIZE_BY2];  // stores FFT windows for averaging
float rawFreqsAvg[FFT_WINDOW_SIZE_BY2];                      // stores the time averaged rawFreqs
float rawFreqsAvgPrev[FFT_WINDOW_SIZE_BY2];                  // stores the previous time averaged rawFreqs
const float _FFT_WINDOW_COUNT = 1.0 / FFT_WINDOW_COUNT;
int averageWindowCount = 0;  // global variable for iterating through circular rawFreqs buffer

float freqs[FFT_WINDOW_SIZE_BY2];      // stores the data from the current fft frequency magnitudes
float freqsPrev[FFT_WINDOW_SIZE_BY2];  // stores the data from the previous fft frequency magnitudes

int FFTPeaks[FFT_WINDOW_SIZE_BY2 >> 1];  // stores the peaks computed by FFT
float FFTPeaksAmp[FFT_WINDOW_SIZE_BY2 >> 1];

int freqBandsIndexes[NUM_FREQ_BANDS];  // stores FFT bin indexes corresponding to freqBands array

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
float cos_wave_w[AUD_OUT_BUFFER_SIZE];

const int MAX_NUM_WAVES = DET_MODE == 'b' ? NUM_FREQ_BANDS : MAX_NUM_PEAKS;
const int MAX_AMP_SUM = DET_MODE == 'b' ? FFT_MAX_SUM : MAX_NUM_WAVES * FFT_MAX_AMP;
// sine wave for storing pre-generated values of a sine wave at SAMPLING_FREQ sample rate
float sin_wave[SAMPLING_FREQ];
// float cos_wave[SAMPLING_FREQ];
// float tri_wave[SAMPLING_FREQ];
// float sqr_wave[SAMPLING_FREQ];
// float saw_wave[SAMPLING_FREQ];

// stores the sine wave frequencies and amplitudes to synthesize
int sin_waves_freq[AUD_OUT_CH][MAX_NUM_WAVES];
int sin_waves_amp[AUD_OUT_CH][MAX_NUM_WAVES];

// stores the number of sine waves in each channel
int num_waves[AUD_OUT_CH];

// stores the position of wave
int sin_wave_idx = 0;

// scratchpad array used for signal synthesis
float generateAudioBuffer[AUD_OUT_CH][GEN_AUD_BUFFER_SIZE];
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
// rolling buffer for outputting synthesized signal
volatile int AUD_OUT_BUFFER[AUD_OUT_CH][AUD_OUT_BUFFER_SIZE];

// audio input and output buffer index
volatile int AUD_IN_BUFFER_IDX = 0;
volatile int AUD_OUT_BUFFER_IDX = 0;

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

#ifdef DEBUG
unsigned long loop_time = 0;  // used for printing time main loop takes to execute in debug mode
#endif

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

  // calculate indexes associated to frequency bands
  calculatefreqBands();

  // setup detection algorithm mode
  switch (DET_MODE) {
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
  switch (DET_DATA) {
    case 'a':
      detFuncData = rawFreqsAvg;
      break;
    case 'c':
      detFuncData = freqs;
      break;
    default:
      Serial.println("USING DEFAULT DETECTION DATA: freqs");
      detFuncData = freqs;
      break;
  }

  // attach frequency/amplitude scaling function
  switch (SCALING_MODE) {
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
  pinMode(AUD_OUT_PIN, OUTPUT);

  Serial.println("Setup complete");

  delay(1000);

  // setup timer interrupt for audio sampling
  SAMPLING_TIMER = timerBegin(0, 80, true);                        // setting clock prescaler 1MHz (80MHz / 80)
  timerAttachInterrupt(SAMPLING_TIMER, &ON_SAMPLING_TIMER, true);  // attach interrupt function
  timerAlarmWrite(SAMPLING_TIMER, sampleDelayTime, true);          // trigger interrupt every @sampleDelayTime microseconds
  timerAlarmEnable(SAMPLING_TIMER);                                // enabled interrupt
}

/*/
########################################################
  Loop
########################################################
/*/

void loop() {
  if (AUD_IN_BUFFER_FULL()) {
    // in debug mode, print total time spent in loop, optionally interrupt timer can be disabled
    #ifdef DEBUG
    // timerAlarmDisable(SAMPLING_TIMER); // optionally disable interrupt timer during debug mode
    loop_time = micros();
    #endif

    // fft data processing, returns num sine waves to synthesize
    processData();

    // generate audio for the next audio window
    generateAudioForWindow();

    #ifdef DEBUG
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
    #endif
  }
}

/*/
########################################################
  Process data
########################################################
/*/

void processData() {
  // copy values from AUD_IN_BUFFER to vReal array
  setupFFT();

  // restores AUD_IN and AUD_OUT buffer positions
  RESET_AUD_IN_OUT_IDX();

  // perform fft
  FFT.DCRemoval();                                  // DC Removal to reduce noise
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
  FFT.Compute(FFT_FORWARD);                         // Compute FFT
  FFT.ComplexToMagnitude();                         // Compute frequency magnitudes

  // copy values calculated by FFT to freqs and rawFreqs
  storeFreqs();

  float mean = 0;
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    mean += detFuncData[i];
  }
  mean /= FFT_WINDOW_SIZE_BY2;

  // noise flooring based on mean of data and a threshold
  noiseFloor(detFuncData, FFT_NOISE_THRESH);

  detectionFunc(detFuncData);

  // if @FREQ_MAX_AMP_DELTA was enabled, returns the index of the amplitude of most change within set threshold between FFT windows as well as the magnitude of change which is passed as a reference
  float maxAmpDeltaMag = 0.0;
  int maxAmpDeltaIdx = FREQ_MAX_AMP_DELTA ? frequencyMaxAmplitudeDelta(freqs, freqsPrev, FREQ_MAX_AMP_DELTA_START, FREQ_MAX_AMP_DELTA_END, maxAmpDeltaMag) : -1;

  resetSinWaves(0);
  assignSinWaves(assignSinWavesFreq, assignSinWavesAmp, assignSinWavesNum);

  // weight frequencies based on passed weighting function, returns the total sum of the amplitudes and the index of the max amplitude which is passed by reference
  int maxAmpIdx = 0;
  int totalEnergy = scaleAmplitudeFrequency(maxAmpIdx, maxAmpDeltaIdx, maxAmpDeltaMag, num_waves[0]);

  // a very basic way to look for high hats, simply just look for noise above a certain threshold throughout the spectrum
  int noiseC = 0;
  int highHatEnergy = 0;
  int highHatThresh = mean * 0.5;
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    if (freqs[i] > highHatThresh) {
      noiseC += 1;
      highHatEnergy += freqs[i];
    }
  }
  // if the noise is present in at least 80% the spectrum, then consider that a high hat has been detected
  if (noiseC > (FFT_WINDOW_SIZE_BY2 * 0.8) && SCALING_MODE != 'm') {
    int minAmp = sin_waves_amp[0][maxAmpIdx];
    int minAmpIdx = maxAmpIdx;
    // find the least amplitude sine waves array to replaced with dedicated freqeuncy for high hat representation
    for (int i = 0; i < num_waves[0]; i++) {
      if (sin_waves_amp[0][i] < minAmp) {
        minAmp = sin_waves_amp[0][i];
        minAmpIdx = i;
      }
    }
    // scale high hat energy by desired value
    highHatEnergy *= 4;
    // ensure that the total energy is accurate
    totalEnergy += highHatEnergy - sin_waves_amp[0][minAmpIdx];
    sin_waves_amp[0][minAmpIdx] = highHatEnergy;
    sin_waves_freq[0][minAmpIdx] = 90;
  }

  // maps the amplitudes so that their total sum is no more or equal to 127
  mapAmplitudes();
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
    addSinWave(freqData[i], ampData[i], 0);
  }
}

// iterates through sine waves and scales frequencies/amplitudes based on frequency bands and the frequency of max amplitude change
long scaleAmplitudeFrequency(int &maxAmpIdx, int maxAmpDeltaIdx, float maxAmpDeltaMag, int numWaves) {
  long sum = 0;
  int maxAmp = 0;
  // iterate through sin waves
  for (int i = 0; i < numWaves; i++) {
    int amplitude = sin_waves_amp[0][i];
    int frequency = sin_waves_freq[0][i];

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
    sin_waves_amp[0][i] = amplitude;
    sin_waves_freq[0][i] = frequency;
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
      return;
    }
  }
  freq = 0;
  amp = 0;
}

// scales frequencies based on values in between frequency bands stored in freqBandsMapK, amplitude weighting based on freqBandsK
void ampFreqWeightingB(int &freq, int &amp) {
  for (int i = 0; i < NUM_FREQ_BANDS; i++) {
    if (freq <= freqBands[i]) {
      amp *= freqBandsK[i];
      freq = map(freq, i > 0 ? freqBands[i - 1] : 1, freqBands[i], i > 0 ? freqBandsMapK[i - 1] : 25, freqBandsMapK[i]);
      return;
    }
  }
  freq = 0;
  amp = 0;
}

// scales frequency by mapping sub-bass to 30Hz, and the full spectrum to the desired range
void ampFreqWeightingC(int &freq, int &amp) {
  amp *= c_weightingFilter(freq);

  if (freq < 60) {
    freq = 35;
  } else {
    // normalize between 0 and 1.0, where 0.0 corresponds to 60Hz, while SAMPLING_FREQ / 2 Hz corresponds to 1.0
    float freq_n = (freq - 60) / float(SAMPLING_FREQ * 0.5 - 60);
    // broaden range for lower frequencies
    freq_n = pow(freq_n, 0.2);
    // multiply by maximum desired value
    freq = round(freq_n * 120);
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
  amp *= freqBandsK[band];
  switch (band) {
    case 0:
      freq = 25;
      break;
    case 1:
      freq = round(freq * 0.25) + 25;
      break;
    case 2:
      freq = round(freq * 0.125) + 25;
      break;
    case 3:
      freq = round(freq * 0.0625) + 15;
      break;
    default:
      freq = round(freq * 0.03125);
      break;
  }
}

// mirror mode scaling, nothing happens to frequencies and amplitudes
void ampFreqWeightingM(int &freq, int &amp) {
  return;
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
  amplitude weighting filters based on equal loudness contours (https://www.cross-spectrum.com/audio/weighting.html)
########################################################
/*/

float a_weightingFilter(int frequency) {
  float f_2 = pow(frequency * 0.1, 2);
  float f_4 = pow(frequency * 0.1, 4);

  return (c_weighting_k1 * f_4) / ((f_2 + c_weighting_k2) * (f_2 + c_weighting_k1) * pow(f_2 + a_weighting_k1, 0.5) * pow(f_2 + a_weighting_k2, 0.5));
}

float b_weightingFilter(int frequency) {
  float f_2 = pow(frequency * 0.1, 2);
  float f_3 = pow(frequency * 0.1, 3);

  return (c_weighting_k1 * f_3) / ((f_2 + c_weighting_k2) * (f_2 + c_weighting_k1) * pow(f_2 + b_weighting_k1, 0.5));
}

float c_weightingFilter(int frequency) {
  float f = pow(frequency * 0.1, 2);

  return (c_weighting_k1 * f) / ((f + c_weighting_k2) * (f + c_weighting_k1));
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

// store the current window into freqs and rawFreqs for averaging windows to reduce noise and produce a cleaner spectrogram for signal processing
void storeFreqs() {
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    float data = vReal[i] * freqWidth;
    // store oldfreqs into prevfreqs, and newly computed FFT results in freqs
    freqsPrev[i] = freqs[i];
    freqs[i] = data;

    // store data in rawFreqs array
    rawFreqs[averageWindowCount][i] = data;
    float amplitudeSum = 0.0;
    for (int j = 0; j < FFT_WINDOW_COUNT; j++) {
      amplitudeSum += rawFreqs[j][i];
    }
    // store old averaged window in rawFreqsAvgPrev, and the current averaged window in rawFreqsAvg
    rawFreqsAvgPrev[i] = rawFreqsAvg[i];
    rawFreqsAvg[i] = amplitudeSum * _FFT_WINDOW_COUNT;

    //Serial.printf("idx: %d, averaged: %.2f, data: %.2f\n", i, vReal[i], freqs[i]);
  }
  averageWindowCount += 1;
  if (averageWindowCount == FFT_WINDOW_COUNT) { averageWindowCount = 0; }
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
int frequencyMaxAmplitudeDelta(float *data, float *prevData, int minFreq, int maxFreq, float &changeMagnitude) {
  // calculate indexes in FFT bins correspoding to minFreq and maxFreq
  int minIdx = round(minFreq * freqWidth);
  int maxIdx = round(maxFreq * freqWidth);
  // restore global varialbes
  int maxAmpChange = 0;
  int maxAmpChangeIdx = -1;
  // iterate through data* and prevData* to find the amplitude with most change
  for (int i = minIdx; i < maxIdx; i++) {
    if (i == FFT_WINDOW_SIZE_BY2) { break; }
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

// calculates indexes corresponding to frequency bands
void calculatefreqBands() {
  // converting from frequency to index
  for (int i = 0; i < NUM_FREQ_BANDS; i++) {
    freqBandsIndexes[i] = ceil(freqBands[i] * freqWidth);
  }
}

// slices fft data into bands, storing the maximum peak in the band and averaged sum of the total energy in the band
void breadslicer(float *data) {
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
      breadslicerSums[sliceCount] = sliceSum;      // store slice sum
      breadslicerPeaks[sliceCount] = sliceMaxIdx;  // store slice peak
      sliceSum = 0;                                // reset variables
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
    #ifdef DEBUG
    Serial.printf("CANNOT ADD SINE WAVE, CHANNEL %d ISN'T DEFINED\n", ch);
    #endif
    return;
  }
  // ensures that the sum of waves in the channels is no greater than MAX_NUM_WAVES
  int num_waves_count = 0;
  for (int c = 0; c < AUD_OUT_CH; c++) {
    num_waves_count += num_waves[c];
  }

  if (num_waves_count == MAX_NUM_WAVES) {
    #ifdef DEBUG
    Serial.println("CANNOT ASSIGN MORE WAVES, CHANGE MAX_NUM_WAVES!");
    #endif
    return;
  }

  sin_waves_amp[ch][num_waves[ch]] = amp;
  sin_waves_freq[ch][num_waves[ch]] = freq;
  num_waves[ch] += 1;
}

// removes a sine wave at specified index and channel
void removeSinWave(int idx, int ch) {
  if (ch > AUD_OUT_CH - 1) {
    #ifdef DEBUG
    Serial.printf("CANNOT REMOVE WAVE, CHANNEL %d ISN'T DEFINED\n", ch);
    #endif
    return;
  }
  if (idx >= num_waves[ch]) {
    #ifdef DEBUG
    Serial.printf("CANNOT REMOVE WAVE, INDEX %d DOESN'T EXIST!\n", idx);
    #endif
    return;
  }

  for (int i = idx; i < --num_waves[ch]; i++) {
    sin_waves_amp[ch][i] = sin_waves_amp[ch][i + 1];
  }
}

// modify sine wave at specified index and channel to desired frequency and amplitude
void modifySinWave(int idx, int ch, int freq, int amp) {
  if (ch > AUD_OUT_CH - 1) {
    #ifdef DEBUG
    Serial.printf("CANNOT MODIFY WAVE, CHANNEL %d ISN'T DEFINED\n", ch);
    #endif
    return;
  }
  if (idx >= num_waves[ch]) {
    #ifdef DEBUG
    Serial.printf("CANNOT MODIFY WAVE, INDEX %d DOESN'T EXIST!\n", idx);
    #endif
    return;
  }
  sin_waves_amp[ch][idx] = amp;
  sin_waves_freq[ch][idx] = freq;
}

// sets all sine waves and frequencies to 0 on specified channel
void resetSinWaves(int ch) {
  if (ch > AUD_OUT_CH - 1) {
    #ifdef DEBUG
    Serial.printf("CANNOT RESET WAVES, CHANNEL %d ISN'T DEFINED\n", ch);
    #endif
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
        AUD_OUT_BUFFER[c][generateAudioOutIdx] = max(0, min(255, int(round(generateAudioBuffer[c][generateAudioIdx] + 128.0))));
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

// restores AUD_IN_BUFFER_IDX, and ensures AUD_OUT_BUFFER is synchronized
void RESET_AUD_IN_OUT_IDX() {
  AUD_OUT_BUFFER_IDX += AUD_IN_BUFFER_SIZE;
  if (AUD_OUT_BUFFER_IDX >= AUD_OUT_BUFFER_SIZE) AUD_OUT_BUFFER_IDX = 0;
  AUD_IN_BUFFER_IDX = 0;
}

// outputs sample from AUD_OUT_BUFFER to DAC and reads sample from ADC to AUD_IN_BUFFER
void AUD_IN_OUT() {
  if (AUD_IN_BUFFER_FULL()) return;

  int AUD_OUT_IDX = AUD_OUT_BUFFER_IDX + AUD_IN_BUFFER_IDX;
  dacWrite(AUD_OUT_PIN, AUD_OUT_BUFFER[0][AUD_OUT_IDX]);
  
  AUD_IN_BUFFER[AUD_IN_BUFFER_IDX] = adc1_get_raw(AUD_IN_PIN);
  AUD_IN_BUFFER_IDX += 1;
}