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

#define USE_FFT  // comment to disable FFT / audio input

// audio sampling stuff
#define AUD_IN_PIN ADC1_CHANNEL_6 // corresponds to ADC on A2
#define AUD_OUT_PIN_L A0
#define AUD_OUT_PIN_R A1

#define FFT_WINDOW_SIZE 256   // size of a recording window, this value MUST be a power of 2
#define SAMPLING_FREQ 10000   // The audio sampling and audio output rate, with the ESP32 the fastest we could sample at is just under 24kHz, but
                // bringing this down leaves more processing time for FFT, signal processing functions, and synthesis

#define FFT_MAX_AMP 300   // the maximum frequency magnitude of an FFT bin, this is multiplied by the total number of waves being synthesized for volume representation

#define NUM_FREQ_WINDOWS 8  // the number of frequency windows to store in circular buffer freqs

#define FREQ_MAX_AMP_DELTA_MIN 50     // the min threshold of change in amplitude to be considered significant by the frequencyMaxAmplitudeDelta() function
#define FREQ_MAX_AMP_DELTA_MAX 500    // the max threshold of change in amplitude
#define FREQ_MAX_AMP_DELTA_K 16.0      // weight for amplitude of most change

#define MAX_NUM_PEAKS 7  // the maximum number of peaks to look for with the findMajorPeaks() function

#define MAX_NUM_WAVES 8  // maximum number of waves to synthesize (all channels combined)
#define NUM_OUT_CH 2  // number of audio channels to synthesize

#define MIRROR_MODE // comment to disable mirror mode

#define DEBUG   // uncomment for debug mode, the time taken in loop is printed (in microseconds), along with the assigned sine wave frequencies and amplitudes

/*/
########################################################
  Check for errors in directives
########################################################
/*/

#if MAX_NUM_WAVES <= 0
#error MAX_NUM_WAVES MUST BE AT LEAST 1
#endif

#if NUM_OUT_CH <= 0
#error NUM_OUT_CH MUST BE AT LEAST 1
#endif

/*/
########################################################
  Variables used for interrupt and function to call when interrupt is triggered
########################################################
/*/

const int AUD_IN_BUFFER_SIZE = FFT_WINDOW_SIZE;  
const int AUD_OUT_BUFFER_SIZE = FFT_WINDOW_SIZE * 2;

// used to store recorded samples for a gien window
volatile int AUD_IN_BUFFER[AUD_IN_BUFFER_SIZE];
// rolling buffer for outputting synthesized signal
volatile int AUD_OUT_BUFFER[NUM_OUT_CH][AUD_OUT_BUFFER_SIZE];

// audio input and output buffer index
volatile int AUD_IN_BUFFER_IDX = 0;
volatile int AUD_OUT_BUFFER_IDX = 0;

hw_timer_t *SAMPLING_TIMER = NULL;

void IRAM_ATTR AUD_IN_OUT(void) {
  if (AUD_IN_BUFFER_FULL()) return;

  int AUD_OUT_IDX = AUD_OUT_BUFFER_IDX + AUD_IN_BUFFER_IDX;
  dacWrite(AUD_OUT_PIN_L, AUD_OUT_BUFFER[0][AUD_OUT_IDX]);
  #if NUM_OUT_CH == 2
  dacWrite(AUD_OUT_PIN_R, AUD_OUT_BUFFER[1][AUD_OUT_IDX]);
  #endif

  #ifdef USE_FFT
  AUD_IN_BUFFER[AUD_IN_BUFFER_IDX] = adc1_get_raw(AUD_IN_PIN);
  #endif
  AUD_IN_BUFFER_IDX += 1;
}

/*/
########################################################
  FFT
########################################################
/*/

#ifdef USE_FFT
float vReal[FFT_WINDOW_SIZE];  // vReal is used for input at SAMPLING_FREQ and receives computed results from FFT
float vImag[FFT_WINDOW_SIZE];  // used to store imaginary values for computation

arduinoFFT FFT = arduinoFFT(vReal, vImag, FFT_WINDOW_SIZE, SAMPLING_FREQ);  // Object for performing FFT's


/*/
########################################################
  Stuff relevant to FFT signal processing
########################################################
/*/

const int sampleDelayTime = 1000000 / SAMPLING_FREQ;

// FFT_SIZE_BY_2 is FFT_WINDOW_SIZE / 2. We are sampling at double the frequency we are trying to detect, therefore only half of the FFT bins are used for analysis post FFT
const int FFT_WINDOW_SIZE_BY2 = int(FFT_WINDOW_SIZE) >> 1;

const float freqRes = float(SAMPLING_FREQ) / FFT_WINDOW_SIZE;  // the frequency resolution of FFT with the current window size

const float freqWidth = 1.0 / freqRes; // the width of an FFT bin

float freqs[NUM_FREQ_WINDOWS][FFT_WINDOW_SIZE_BY2];   // stores frequency magnitudes computed by FFT over NUM_FREQ_WINDOWS
int freqsWindow = 0;                                  // time position of freqs buffer
int freqsWindowPrev = 0;

float* freqsCurrent = NULL;
float* freqsPrevious = NULL;

int FFTPeaks[FFT_WINDOW_SIZE_BY2 >> 1]; // stores the peaks computed by FFT
float FFTPeaksAmp[FFT_WINDOW_SIZE_BY2 >> 1];

const int bassIdx = 120 * freqWidth; // FFT bin index of frequency ~200Hz

const int MAX_AMP_SUM = MAX_NUM_WAVES * FFT_MAX_AMP;
#endif

/*/
########################################################
  Variables used for generating an audio output
########################################################
/*/

const int GEN_AUD_BUFFER_SIZE = FFT_WINDOW_SIZE * 3;

const float _SAMPLING_FREQ = 1.0 / SAMPLING_FREQ;

// a cosine wave for modulating sine waves
float cos_wave_w[AUD_OUT_BUFFER_SIZE];

// sine wave for storing pre-generated values of a sine wave at SAMPLING_FREQ sample rate
float sin_wave[SAMPLING_FREQ];
// float cos_wave[SAMPLING_FREQ];
// float tri_wave[SAMPLING_FREQ];
// float sqr_wave[SAMPLING_FREQ];
// float saw_wave[SAMPLING_FREQ];

// structure for wave
struct wave 
{
  wave(int amp, int freq, int phase): freq(freq), amp(amp), phase(phase) {}
  wave(): amp(0), freq(0), phase(0) {}
  int freq;
  int amp;
  int phase;
};

struct wave_map
{
  wave_map(int ch, int amp, int freq, int phase): ch(ch), valid(1), w(wave(freq, amp, phase)) {}
  wave_map(): ch(0), valid(0), w(wave(0, 0, 0)) {}
  int ch;
  bool valid;
  wave w;
}; 

wave_map waves_map[MAX_NUM_WAVES];

// stores the sine wave frequencies and amplitudes to synthesize
wave waves[NUM_OUT_CH][MAX_NUM_WAVES];

// stores the number of sine waves in each channel
int num_waves[NUM_OUT_CH];

// stores the position of wave
int sin_wave_idx = 0;

// scratchpad array used for signal synthesis
float generateAudioBuffer[NUM_OUT_CH][GEN_AUD_BUFFER_SIZE];
// stores the current index of the scratch pad audio output buffer
int generateAudioIdx = 0;
// used for copying final synthesized values from scratchpad audio output buffer to volatile audio output buffer
int generateAudioOutIdx = 0;

/*/
########################################################
  Setup
########################################################
/*/

void setup() {
  // set baud rate
  Serial.begin(115200);
  // wait for Serial to set up
  while (!Serial);

  delay(4000);

  init();
}

/*/
########################################################
  Loop
########################################################
/*/

void loop() {
  if (ready()) {
    //unsigned long loopt = micros();
    pullSamples();

    resume();

    performFFT();

    processData();

    printWaves();

    generateAudioForWindow();
    //Serial.println(micros() - loopt);
  }
}



/*/
########################################################
  General functions that are used in setup and loop
########################################################
/*/

// initialization of input and output pins, ISR and buffers
void init(void) {
  analogReadResolution(12);

  // setup pins
  //pinMode(AUD_IN_PINA, INPUT);
  pinMode(AUD_OUT_PIN_L, OUTPUT);
  pinMode(AUD_OUT_PIN_R, OUTPUT);

  delay(1000);

  // adc1_config_width(ADC_WIDTH_BIT_12);
  // adc1_config_channel_atten(AUD_IN_PIN, ADC_ATTEN_DB_0);

  delay(1000);

  // initialize generate audio and output buffers to 0, not necassary but helps prevent glitches when program boots
  initAudio();
  // attach timer interrupt
  setupISR();

  disableAudio();
  Serial.println("Vibrosonics setup complete");
  Serial.printf("SAMPLE RATE: %d Hz\tWINDOW SIZE: %d\tSPEED AND RESOLUTION: %.1f Hz\tTIME PER WINDOW: %.1f ms\tAUD IN OUT DELAY: %.1f ms", SAMPLING_FREQ, FFT_WINDOW_SIZE, freqRes, sampleDelayTime * FFT_WINDOW_SIZE * 0.001, 2 * sampleDelayTime * FFT_WINDOW_SIZE * 0.001);
  Serial.println();
  delay(1000);
  enableAudio();
}

// returns true when the audio input buffer fills. If not using FFT then returns true when modifications to waves can be made
bool ready(void) {
  return AUD_IN_BUFFER_FULL();
}

// call resume to continue audio input and/or output
void resume(void) {
  // reset AUD_IN_BUFFER_IDX to 0, so interrupt can continue to perform audio input and output
  RESET_AUD_IN_OUT_IDX();
}

/*/
########################################################
  Functions related to FFT analysis
########################################################
/*/

void pullSamples() {
  for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
    vReal[i] = AUD_IN_BUFFER[i];
    // Serial.println(vReal[i]);
    // delay(1);
  }
}

void performFFT() {
  for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
    vImag[i] = 0.0;
  }
  
  // use arduinoFFT
  FFT.DCRemoval();                  // DC Removal to reduce noise
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
  FFT.Compute(FFT_FORWARD);             // Compute FFT
  FFT.ComplexToMagnitude();             // Compute frequency magnitudes
}

// FFT data processing
void processData(void) {  
  resetAllWaves();

  // copy values calculated by FFT to freqs
  storeFreqs();

  // noise flooring based on a threshold
  //if (getMean(freqsCurrent, FFT_WINDOW_SIZE_BY2) < 30.0) return;

  noiseFloor(freqsCurrent, 20.0);
  
  // find frequency of most change and its magnitude between the current and previous window
  #ifndef MIRROR_MODE
  float freqOfMaxChangeMag = 0.0;
  int freqOfMaxChange = frequencyMaxAmplitudeDelta(freqsCurrent, freqsPrevious, 200, 4000, freqOfMaxChangeMag);

  // add wave for frequency of max amplitude change
  if (freqOfMaxChange > 0) {
    int amplitude = sumOfPeak(freqsCurrent, freqOfMaxChange) + sumOfPeak(freqsPrevious, freqOfMaxChange);
    int frequency = interpolateAroundPeak(freqsCurrent, freqOfMaxChange);
    frequency = freqWeighting(frequency);
    #if AUD_OUT_CH == 1
    addWave(0, frequency, amplitude * freqOfMaxChangeMag);
    #else
    addWave(1, frequency, amplitude * freqOfMaxChangeMag);
    #endif
  }
  #endif

  // finds peaks in data, stores index of peak in FFTPeaks and Amplitude of peak in FFTPeaksAmp
  findMajorPeaks(freqsCurrent);

  // assign sine waves based on data found by major peaks
  assignWaves(FFTPeaks, FFTPeaksAmp, FFT_WINDOW_SIZE_BY2 >> 1);
  mapAmplitudes(MAX_AMP_SUM);
}


void storeFreqs(void) {
  freqsWindowPrev = freqsWindow;
  freqsWindow += 1;
  if (freqsWindow >= NUM_FREQ_WINDOWS) freqsWindow = 0;
  freqsPrevious = freqs[freqsWindowPrev];
  freqsCurrent = freqs[freqsWindow];
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    freqsCurrent[i] = vReal[i] * freqWidth;
  }
}

void noiseFloor(float *data, float threshold) {
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    if (data[i] < threshold) {
      data[i] = 0.0;
    }
  }
}

float getMean(float *data, int size) {
  float sum = 0.0;
  for (int i = 0; i < size; i++) {
    sum += data[i];
  }
  return sum != 0.0 ? sum / size : 0.0;
}

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

// finds the frequency of most change within minFreq and maxFreq, returns the index of the frequency of most change, and stores the magnitude of change (between 0.0 and FREQ_MAX_AMP_DELTA_K) in magnitude reference
int frequencyMaxAmplitudeDelta(float *data, float *prevData, int minFreq, int maxFreq, float &magnitude) {
  // calculate indexes in FFT bins correspoding to minFreq and maxFreq
  int minIdx = round(minFreq * freqWidth);
  if (minIdx == 0) minIdx = 1;
  int maxIdx = round(maxFreq * freqWidth);
  if (maxIdx > FFT_WINDOW_SIZE_BY2 - 1) maxIdx = FFT_WINDOW_SIZE_BY2 - 1;
  // restore global varialbes
  int maxAmpChange = 0;
  int maxAmpChangeIdx = -1;
  // iterate through data* and prevData* to find the amplitude with most change
  for (int i = minIdx; i < maxIdx; i++) {
    // calculate the change between this amplitude and previous amplitude
    int sumAroundDataI = sumOfPeak(data, i);
    int sumAroundPrevDataI = sumOfPeak(prevData, i);
    int currAmpChange = abs(sumAroundDataI - sumAroundPrevDataI);
    // find the most dominant amplitude change and store in maxAmpChangeIdx, store the magnitude of the change in maxAmpChange
    if ((currAmpChange >= FREQ_MAX_AMP_DELTA_MIN) && (currAmpChange <= FREQ_MAX_AMP_DELTA_MAX) && (currAmpChange > maxAmpChange)) {
      maxAmpChange = currAmpChange;
      maxAmpChangeIdx = i;
    }
  }
  magnitude = maxAmpChange * (1.0 / FREQ_MAX_AMP_DELTA_MAX) * FREQ_MAX_AMP_DELTA_K;
  return maxAmpChangeIdx;
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

int sumOfPeak(float *data, int indexOfPeak) {
  if (indexOfPeak > 0 && indexOfPeak < FFT_WINDOW_SIZE_BY2 - 1) {
    return data[indexOfPeak - 1] + data[indexOfPeak] + data[indexOfPeak + 1];
  }
  return 0;
}

/*/
########################################################
  Functions relating to assigning sine waves and mapping their frequencies and amplitudes
########################################################
/*/

// assigns the frequencies and amplitudes found by majorPeaks to sine waves
void assignWaves(int* freqData, float* ampData, int size) {  
  // assign sin_waves and freq/amps that are above 0, otherwise skip
  for (int i = 0; i < size; i++) {
    // skip storing if ampData is 0, or freqData is 0
    if (ampData[i] == 0.0 || freqData[i] == 0) continue;
    // assign frequencies below bass to left channel, otherwise to right channel
    if (freqData[i] <= bassIdx) {
      int freq = freqData[i];
      // if the difference of energy around the peak is greater than threshold
      if (abs(freqsCurrent[freqData[i] - 1] - freqsCurrent[freqData[i] + 1]) > 100) {
        // assign frequency based on whichever side is greater
        freq = freqsCurrent[freqData[i] - 1] > freqsCurrent[freqData[i] + 1] ? (freqData[i] - 0.5) : (freqData[i] + 0.5);
      }
      addWave(0, freq * freqRes, ampData[i], 0);
    } else {
      int interpFreq = interpolateAroundPeak(freqsCurrent, freqData[i]);

      #ifndef MIRROR_MODE
      interpFreq = freqWeighting(interpFreq);
      #endif

      #if NUM_OUT_CH == 1
      addWave(0, interpFreq, ampData[i], 0);
      #else
      addWave(1, interpFreq, ampData[i], 0);
      #endif
    }
  }
}

int freqWeighting(int freq) {
  // normalize between 0 and 1.0, where 0.0 corresponds to 120Hz, while SAMPLING_FREQ / 2 Hz corresponds to 1.0
  float freq_n = (freq - 120) / float(SAMPLING_FREQ * 0.5 - 120);
  // broaden range for lower frequencies
  freq_n = pow(freq_n, 0.2);
  // multiply by maximum desired value
  freq = round(freq_n * 120);
}

/*/
########################################################
  Wave assignment and modification
########################################################
/*/

bool isValidId(int id) {
  if (id >= 0 && id < MAX_NUM_WAVES) {
    if (waves_map[id].valid) return 1;
  }
  #ifdef DEBUG
  Serial.printf("WAVE ID %d INVALID! ", id);
  #endif
  return 0;
}

bool isValidChannel(int ch) {
  if (ch < 0 || ch > NUM_OUT_CH - 1) {
    #ifdef DEBUG
    Serial.printf("CHANNEL %d IS INVALID! ", ch);
    #endif
    return 0;
  }
  return 1;
}

int addWave(int ch, int freq, int amp, int phase) {
  if (!isValidChannel(ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT ADD WAVE");
    #endif
    return -1;
  }

  if (freq < 0 || phase < 0) {
    #ifdef DEBUG
    Serial.println("CANNOT ADD WAVE, FREQ AND PHASE MUST BE POSTIVE!");
    #endif
    return -1;
  } 

  int unmapped_idx = -1;
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    if (waves_map[i].valid == 0) {
      unmapped_idx = i;
      break;
    }
  }

  if (unmapped_idx == -1) {
    #ifdef DEBUG
    Serial.println("CANNOT ADD WAVE, CHANGE MAX_NUM_WAVES!");
    #endif
    return unmapped_idx;
  }
  waves_map[unmapped_idx].valid = 1;
  waves_map[unmapped_idx] = wave_map(ch, freq, amp, phase);
  return unmapped_idx;
}

void removeWave(int id) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT REMOVE WAVE");
    #endif
    return;
  }
  waves_map[id].valid = 0;
  waves_map[id].w = wave();
}

void modifyWave(int id, int freq, int amp, int phase, int ch) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT MODIFY WAVE");
    #endif
    return;
  }

  if (!isValidChannel(ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT MODIFY WAVE");
    #endif
    return;
  }

  if (freq < 0 || phase < 0) {
    #ifdef DEBUG
    Serial.println("CANNOT MODIFY WAVE, FREQ AND PHASE MUST BE POSTIVE!");
    #endif
    return;
  } 

  waves_map[id].ch = ch;
  waves_map[id].w = wave(freq, amp, phase);
}

void resetWaves(int ch) {
  if (!isValidChannel(ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT RESET WAVES");
    #endif
    return;
  }

  // restore amplitudes and frequencies on ch, invalidate wave
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    if (waves_map[i].valid == 0) continue;
    if (waves_map[i].ch != ch) continue;
    waves_map[i].valid = 0;
    waves_map[i].w = wave();
  }
}

void resetAllWaves(void) {
  // restore amplitudes and frequencies on ch, invalidate wave
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    if (waves_map[i].valid == 0) continue;
    waves_map[i].valid = 0;
    waves_map[i].w = wave();
  }
}

int getNumWaves(int ch) { 
  if (!isValidChannel(ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT GET NUM WAVES");
    #endif
    return -1;
  }
  // restore amplitudes and frequencies on ch, invalidate wave
  int count = 0;
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    if (waves_map[i].valid == 0) continue;
    if (waves_map[i].ch != ch) continue;
    count += 1;
  }
  return count;
}

int getFreq(int id) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT GET FREQUENCY");
    #endif
    return -1;
  }

  return waves_map[id].w.freq;
}

int getAmp(int id) { 
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT GET AMPLITUDE");
    #endif
    return -1;
  }

  return waves_map[id].w.freq;
}

int getPhase(int id) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT GET PHASE");
    #endif
    return -1;
  }

  return waves_map[id].w.phase;
}

int getChannel(int id) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT GET CHANNEL");
    #endif
    return -1;
  }
  return waves_map[id].ch;
}

bool setFreq(int id, int freq) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT SET FREQUENCY");
    #endif
    return 0;
  } 

  waves_map[id].w.freq = freq;
  return 1;
}

bool setAmp(int id, int amp) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT SET AMPLITUDE");
    #endif
    return 0;
  } 

  waves_map[id].w.freq = amp;
  return 1;
}

bool setPhase(int id, int phase) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT SET PHASE");
    #endif
    return 0;
  } 

  waves_map[id].w.freq = phase;
  return 1;
}

bool setChannel(int id, int ch) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT SET CHANNEL");
    #endif
    return 0;
  }
  if (!isValidChannel(ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT SET CHANNEL");
    #endif
    return 0;
  }
  waves_map[id].ch = ch;
  return 1;
}

void printWaves(void) {
  for (int c = 0; c < NUM_OUT_CH; c++) {
    Serial.printf("CH %d (F, A, P?): ", c);
    for (int i = 0; i < MAX_NUM_WAVES; i++) {
      if (waves_map[i].valid == 0) continue;
      if (waves_map[i].ch != c) continue;
      Serial.printf("(%03d, %03d", waves_map[i].w.freq, waves_map[i].w.amp);
      if (waves_map[i].w.phase > 0) {
        Serial.printf(", %04d", waves_map[i].w.phase);
      }
      Serial.print(") ");
    }
    Serial.println();
  }
}

// maps amplitudes between 0 and 127 range to correspond to 8-bit (0, 255) DAC on ESP32 Feather mapping is based on the MAX_AMP_SUM
void mapAmplitudes(int minSum) {
  // map amplitudes on both channels
  for (int c = 0; c < NUM_OUT_CH; c++) {
    int ampSum = 0;
    for (int i = 0; i < MAX_NUM_WAVES; i++) {
      if (waves_map[i].valid == 0) continue;
      if (waves_map[i].ch != c) continue;
      ampSum += waves_map[i].w.amp;
    }
    // since all amplitudes are 0, then there is no need to map between 0-127 range.
    if (ampSum == 0) {
      resetWaves(c);
      continue;
    }
    // value to map amplitudes between 0.0 and 1.0 range, the MAX_AMP_SUM will be used to divide unless totalEnergy exceeds this value
    float divideBy = 1.0 / float(ampSum > MAX_AMP_SUM ? ampSum : MAX_AMP_SUM);
    ampSum = 0;
    // map all amplitudes between 0 and 128
    for (int i = 0; i < MAX_NUM_WAVES; i++) {
      if (waves_map[i].valid == 0) continue;
      if (waves_map[i].ch != c) continue;
      int amplitude = waves_map[i].w.amp;
      if (amplitude == 0) continue;
      waves_map[i].w.amp = round(amplitude * divideBy * 127.0);
      ampSum += amplitude;
    }
    // ensures that nothing gets synthesized for this channel
    if (ampSum == 0) resetWaves(c);
  }
}

/*/
########################################################
  Generating values for audio output
########################################################
/*/

float get_wave_val(struct wave w) {
  float sin_wave_freq_idx = (sin_wave_idx * w.freq + w.phase) * _SAMPLING_FREQ;
  int sin_wave_position = (sin_wave_freq_idx - floor(sin_wave_freq_idx)) * SAMPLING_FREQ;
  return w.amp * sin_wave[sin_wave_position];
}

float get_sum_of_channel(int ch) {
  float sum = 0.0;
  for (int s = 0; s < num_waves[ch]; s++) {
    if (waves[ch][s].amp == 0 || (waves[ch][s].freq == 0 && waves[ch][s].phase == 0)) continue;
    sum += get_wave_val(waves[ch][s]);
  }
  return sum;
}

void calculate_windowing_wave(void) {
  float resolution = float(2.0 * PI / AUD_OUT_BUFFER_SIZE);
  for (int i = 0; i < AUD_OUT_BUFFER_SIZE; i++) {
    cos_wave_w[i] = 0.5 * (1.0 - cos(float(resolution * i)));
  }
}

void calculate_waves(void) {
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

void generateAudioForWindow(void) {
  // setup waves array
  for (int c = 0; c < NUM_OUT_CH; c++) {
    num_waves[c] = 0;
  }
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    for (int c = 0; c < NUM_OUT_CH; c++) {
      waves[c][i] = wave();
    }
    if (waves_map[i].valid == 0) continue;
    int ch = waves_map[i].ch;
    waves[ch][num_waves[ch]++] = waves_map[i].w;
  }

  // calculate values for waves
  for (int i = 0; i < AUD_OUT_BUFFER_SIZE; i++) {
    // sum together the sine waves for left channel and right channel
    for (int c = 0; c < NUM_OUT_CH; c++) {
      // add windowed value to the existing values in scratch pad audio output buffer at this moment in time
      generateAudioBuffer[c][generateAudioIdx] += get_sum_of_channel(c) * cos_wave_w[i];
    }

    // copy final, synthesized values to volatile audio output buffer
    if (i < AUD_IN_BUFFER_SIZE) {
    // shifting output by 128.0 for ESP32 DAC, min max ensures the value stays between 0 - 255 to ensure clipping won't occur
      for (int c = 0; c < NUM_OUT_CH; c++) {
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
    for (int c = 0; c < NUM_OUT_CH; c++) {
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
  Timer interrupt and initialization
########################################################
/*/

bool AUD_IN_BUFFER_FULL(void) {
  return !(AUD_IN_BUFFER_IDX < AUD_IN_BUFFER_SIZE);
}

void RESET_AUD_IN_OUT_IDX(void) {
  AUD_OUT_BUFFER_IDX += AUD_IN_BUFFER_SIZE;
  if (AUD_OUT_BUFFER_IDX >= AUD_OUT_BUFFER_SIZE) AUD_OUT_BUFFER_IDX = 0;
  AUD_IN_BUFFER_IDX = 0;
}

void enableAudio(void) {
  timerAlarmEnable(SAMPLING_TIMER);   // enable interrupt
}

void disableAudio(void) {
  timerAlarmDisable(SAMPLING_TIMER);  // disable interrupt
}

void setupISR(void) {
  // setup timer interrupt for audio sampling
  SAMPLING_TIMER = timerBegin(0, 80, true);             // setting clock prescaler 1MHz (80MHz / 80)
  timerAttachInterrupt(SAMPLING_TIMER, &AUD_IN_OUT, true); // attach interrupt function
  timerAlarmWrite(SAMPLING_TIMER, sampleDelayTime, true);     // trigger interrupt every @sampleDelayTime microseconds
  timerAlarmEnable(SAMPLING_TIMER);                 // enabled interrupt
}

void initAudio(void) {
  // calculate values of cosine and sine wave at certain sampling frequency
  calculate_windowing_wave();
  calculate_waves();

  // initialize generate audio and output buffers to 0, not necassary but helps prevent glitches when program boots
  for (int i = 0; i < GEN_AUD_BUFFER_SIZE; i++) {
    for (int c = 0; c < NUM_OUT_CH; c++) {
    generateAudioBuffer[c][i] = 0.0;
      if (i < AUD_OUT_BUFFER_SIZE) {
        AUD_OUT_BUFFER[c][i] = 128;
        if (i < AUD_IN_BUFFER_SIZE) {
          AUD_IN_BUFFER[i] = 2048;
        }
      }
    }
  }

  // reset waves
  resetAllWaves();
}