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

#define AUDIO_INPUT_PIN ADC1_CHANNEL_6 // corresponds to ADC on A2
#define AUDIO_OUTPUT_PIN A0

#define FFT_WINDOW_SIZE 256   // Number of FFT windows, this value must be a power of 2
#define SAMPLING_FREQ 10000   // The audio sampling and audio output rate, with the ESP32 the fastest we could sample at is just under 24kHz, but
                              // bringing this down leaves more processing time for FFT, signal processing functions, and synthesis

#define FFT_FLOOR_THRESH 25.0 // noise floor threshold
#define FFT_MAX_SUM 5000    // the maximum sum of amplitudes of FFT, used for volume representation

#define AVG_WINDOW 4          // number of windows to average to reduce aliasing at lower frequencies

#define MIRROR_MODE 1          // when enabled, frequencies and amplitudes calculated by FFT are directly mapped without any scaling or weighting to represent the "raw" signal

#define MAX_NUM_PEAKS 16       // the maximum number of peaks to look for with the findMajorPeaks() function, also corresponds to how many sine waves are being synthesized

#define MAX_NUM_WAVES 6        // the maximum number of waves to synthesize

#define FREQ_MAX_AMP_DELTA_MIN 10    // the min threshold of change in amplitude to be considered significant by the frequencyMaxAmplitudeDelta() function
#define FREQ_MAX_AMP_DELTA_MAX 150  // the max threshold of change in amplitude
#define FREQ_MAX_AMP_DELTA_K 4.0    // multiplier for amplitude of most change

#define SUB_BASS 60  // frequencies up to here are considered sub bass
#define BASS 200       // frequencies up to this frequency are considered bass
#define LOW_TONES 500   //..
#define MID_TONES 2000   //..
#define HIGH_TONES 4000   //..

// weights for various frequency ranges
#define SUB_BASS_K 8.0
#define BASS_K 1.0
#define LOW_TONES_K 3.0
#define MID_TONES_K 4.0
#define HIGH_TONES_K 3.0
#define VIBRANCE_K 2.0      // vibrance contains the rest of the spectrum (if it exists)

#define NUM_FREQ_BANDS 5
int frequencyBands[NUM_FREQ_BANDS] = { SUB_BASS, BASS, LOW_TONES, MID_TONES, HIGH_TONES };  // stores the frequency bands associated to SUB_BASS through HIGH_TONES

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

const float frequencyResolution = float(SAMPLING_FREQ) / float(FFT_WINDOW_SIZE);  // the frequency resolution of FFT with the current window size

const float frequencyWidth = 1.0 / frequencyResolution; // the width of an FFT bin

float FFTWindows[int(AVG_WINDOW)][FFT_WINDOW_SIZE_BY2]; // stores FFT windows for averaging
float FFTWindowsAvg[FFT_WINDOW_SIZE_BY2];               // stores the averaged FFT windows
float FFTWindowsAvgPrev[FFT_WINDOW_SIZE_BY2];           // stores the previous averaged FFT windows
const float _AVG_WINDOW = 1.0 / AVG_WINDOW;
int averageWindowCount = 0;       // global variable for iterating through circular FFTWindows buffer

float FFTData[FFT_WINDOW_SIZE_BY2];   // stores the data the signal processing functions will use
float FFTDataPrev[FFT_WINDOW_SIZE_BY2]; // stores the previous data used for analysis to find frequency of max amplitude change between consecutive windows

int FFTPeaks[FFT_WINDOW_SIZE_BY2 >> 1]; // stores the peaks computed by FFT
int FFTPeaksAmp[FFT_WINDOW_SIZE_BY2 >> 1];

int slicerIndexes[NUM_FREQ_BANDS + 1]; // stores FFT index's corresponding to frequencyBands array

// stores the peaks in the breadslicer
int breadslicerPeaks[NUM_FREQ_BANDS + 1];
int breadslicerSums[NUM_FREQ_BANDS + 1];

/*/
########################################################
  Variables used for generating an audio output
########################################################
/*/

const int AUDIO_INPUT_BUFFER_SIZE = int(FFT_WINDOW_SIZE);
const int AUDIO_OUTPUT_BUFFER_SIZE = int(FFT_WINDOW_SIZE) * 3;
const int FFT_WINDOW_SIZE_X2 = FFT_WINDOW_SIZE * 2;

// a cosine wave for modulating sine waves
float cos_wave[FFT_WINDOW_SIZE_X2];

// sine wave for storing pre-generated values of a sine wave at SAMPLING_FREQ sample rate
float sin_wave[SAMPLING_FREQ];

// stores the sine wave frequencies and amplitudes to synthesize
int sin_wave_frequency[MAX_NUM_WAVES];
int sin_wave_amplitude[MAX_NUM_WAVES];

// stores the position of sin_wave
int sin_wave_idx = 0;

const float _SAMPLING_FREQ = 1.0 / SAMPLING_FREQ;

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
void SAMPLING_TIMER onTimer(){
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
  calculateBreadfrequencyBands();

  // initialize volatile arrays
  for (int i = 0; i < AUDIO_OUTPUT_BUFFER_SIZE; i++) {
    audioOutputBuffer[i] = 0.0;
    if (i < FFT_WINDOW_SIZE_X2) {
      audioOutputBufferVolatile[i] = 0;
      if (i < FFT_WINDOW_SIZE) {
        audioInputBuffer[i] = 0;
      }
    }
  }

  // setup pins
  pinMode(AUDIO_OUTPUT_PIN, OUTPUT);

  Serial.println("Setup complete");

  delay(1000);

  // while (1) {
  //   if (Serial.available())
  //   {
  //     while (Serial.available())
  //     {
  //       Serial.read();
  //     }
  //     break;
  //   }
  // }

  // setup timer interrupt for audio sampling
  My_timer = timerBegin(0, 80, true);                 // setting clock prescaler 1MHz (80MHz / 80)
  timerAttachInterrupt(My_timer, &onTimer, true);     // attach interrupt function
  timerAlarmWrite(My_timer, sampleDelayTime, true);   // trigger interrupt every @sampleDelayTime microseconds
  timerAlarmEnable(My_timer);     //Just Enable
  timerStart(My_timer);
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
  setupFFTData();
  
  // noise flooring based on @FFT_FLOOR_THRESH
  noiseFloor();

  //breadslicer(FFTData, FFT_WINDOW_SIZE_BY2);
  
  // returns the index of the amplitude of most change within set threshold between consecutive FFT windows
  int maxAmpDeltaIdx = frequencyMaxAmplitudeDelta(FFTData, FFTDataPrev, FFT_WINDOW_SIZE_BY2);
  // finds @MAX_NUM_PEAKS most dominant peaks in FFTData* and returns the total sum of FFTData
  int FFTDataSum = findMajorPeaks(FFTData, FFTPeaks, MAX_NUM_WAVES);

  int numSineWaves = assignSinWaves(FFTPeaks, FFTPeaksAmp, FFT_WINDOW_SIZE_BY2 >> 1);

  //int numSineWaves = assignSinWaves(breadslicerPeaks, breadslicerSums, NUM_FREQ_BANDS + 1);

  // weight frequencies based on frequency bands k and return the sum after weighing amplitudes
  int maxAmpIdx = 0;
  int totalEnergy = scaleAmplitudeFrequency(maxAmpIdx, maxAmpDeltaIdx, numSineWaves);
  if (FFTDataSum > totalEnergy) {
    totalEnergy = FFTDataSum;
  }

  mapAmplitudes(maxAmpIdx, totalEnergy, numSineWaves);

  return numSineWaves;
}

/*/
########################################################
  Functions relating to assigning sine waves and mapping their frequencies and amplitudes
########################################################
/*/

// stores the frequencies and amplitudes found by majorPeaks into separate arrays, and returns the number of sin waves to synthesize
int assignSinWaves(int* ampIdx, int ampData, int size) {
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

// iterates through sine waves and weighs amplitudes based on frequency bands k's
int scaleAmplitudeFrequency(int &maxAmpIdx, int &maxAmpDeltaIdx, int &numWaves) {
  int sum = 0;
  int maxAmp = 0;
  // iterate through sin waves
  for (int i = 0; i < numWaves; i++) {
    int amplitude = sin_wave_amplitude[i];
    int frequency = sin_wave_frequency[i];
    // weighting using equal loudness curve to better represent human audio perception
    //float amp_weighting = a_weightingFilter(frequency);
    //int amplitude = round(amplitude * amp_weighting);
    // find max amplitude
    if (amplitude > maxAmp) {
      maxAmp = amplitude;
      maxAmpIdx = i;
    }

    // if mirror mode is enabled then skip frequency scaling and amplitude weighting to represent the raw input signal
    if (MIRROR_MODE) {
      // interpolate around the peak and convert from index to frequency
      frequency = interpolateAroundPeak(FFTData, FFT_WINDOW_SIZE_BY2, frequency, frequencyResolution);
    } else { 
      // if frequencyOfMaxAmplitudeChange() found a amplitude change above threshold and it exists within a range of a major peak, then weigh this amplitude
      if (maxAmpDeltaIdx >= 0 && frequency == maxAmpDeltaIdx) {
        amplitude = round(amplitude * float(FREQ_MAX_AMP_DELTA_K));
      }
      // map frequencies and weigh amplitudes, frequencies associated with sub bass and bass stay the same other frequencies are scaled
      amplitudeFrequencyWeighting(frequency, amplitude);
    }
    // reassign frequencies and amplitudes
    sin_wave_amplitude[i] = amplitude;
    sin_wave_frequency[i] = frequency;
    // sum the energy associated with the amplitudes
    sum += amplitude;
  }
  return sum;
}

void amplitudeFrequencyWeighting(int &freq, int &amp) {
  // interpolate around the peak and convert from index to frequency
  freq = interpolateAroundPeak(FFTData, FFT_WINDOW_SIZE_BY2, freq, frequencyResolution);
  if (freq <= SUB_BASS) {
    frequency = frequency; 
    amp *= float(SUB_BASS_K);
  } else if (freq <= BASS) {
    freq = freq * 0.5; 
    amp *= float(BASS_K);
  } else if (freq <= LOW_TONES) {
    freq = (freq * 0.125);
    amp *= float(LOW_TONES_K);
  } else if (freq <= MID_TONES) {
    freq = pow(round(freq * 0.0625), 0.96);
    amp *= float(MID_TONES_K);
  } else if (freq <= HIGH_TONES) { 
    freq = round(freq * 0.03125);
    amp *= float(HIGH_TONES_K);
  } else { 
    freq = pow(round(freq * 0.03125), 1.02);
    amp *= float(VIBRANCE_K);
  }
}

// maps amplitudes between 0 and 127 range to correspond to 8-bit ADC on ESP32 Feather
void mapAmplitudes(int &maxAmpIdx, int &totalEnergy, int &numWaves) {
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
    // if assuming single frequency is detected based on the weight of the amplitude,
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
    vReal[i] = float(audioInputBuffer[i]);
    // set imaginary values to 0
    vImag[i] = 0.0;
  }
  // reset flag
  audioInputBufferFull = 0;
}

// Average @AVG_WINDOW FFT windows to reduce noise and produce a cleaner spectrogram for signal processing
// and store the current window into FFTData*
void setupFFTData () {
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    float data = vReal[i] * frequencyWidth;
    // store current window in FFTData
    FFTData[i] = data;
    // store data in FFTWindows array
    FFTWindows[averageWindowCount][i] = data;
    float amplitudeSum = 0.0;
    for (int j = 0; j < AVG_WINDOW; j++) {
      amplitudeSum += FFTWindows[j][i];
    }
    // store averaged window
    FFTWindowsAvg[i] = amplitudeSum * _AVG_WINDOW;
    // Serial.printf("0 1000 %d %d\n", i, int(amplitudeAvg));

    //Serial.printf("idx: %d, average: %.2f, data: %.2f\n", i, vReal[i], FFTData[i]);
  }
  averageWindowCount += 1;
  if (averageWindowCount == AVG_WINDOW) { averageWindowCount = 0; }
}

// noise flooring based on a set threshold
void noiseFloor() {
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    if (FFTData[i] < float(FFT_FLOOR_THRESH)) {
      FFTData[i] = 0.0;
    }
  }
}

// finds the frequency of most change within certain boundaries @FREQ_MAX_AMP_DELTA_MIN and @FREQ_MAX_AMP_DELTA_MAX
int frequencyMaxAmplitudeDelta(float *data, float *prevData, int arraySize) {
  // restore global varialbes
  int maxAmpChange = 0;
  int maxAmpChangeIdx = -1;
  // iterate through data* and prevData* to find the amplitude with most change
  for (int i = 1; i < arraySize; i++) {
    // store the change of between this amplitude and previous amplitude
    int currAmpChange = abs(int(data[i]) - int(prevData[i]));
    // find the most dominant amplitude change and store in maxAmpChangeIdx, store the magnitude of the change in maxAmpChange
    if ((currAmpChange >= FREQ_MAX_AMP_DELTA_MIN && currAmpChange <= FREQ_MAX_AMP_DELTA_MAX) && currAmpChange > maxAmpChange) {
      maxAmpChange = currAmpChange;
      maxAmpChangeIdx = i;
    }
    // assign data to previous data
    prevData[i] = data[i];
  }
  return maxAmpChangeIdx;
}

// finds all the peaks in the fft data* and removes the minimum peaks to contain output to @maxNumPeaks. Returns the total sum of data*
int findMajorPeaks(float* data, int maxNumPeaks) {
  // restore output arrays
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2 >> 1; i++) {
    FFTPeaks[i] = 0;
    FFTPeaksAmp[i] = 0;
  }
  // total sum of data
  int dataSum = data[0] + data[FFT_WINDOW_SIZE_BY2 - 1];
  int peaksFound = 0;
  float maxPeak = 0;
  int maxPeakIdx = 0;
  // iterate through data to find peaks
  for (int f = 1; f < FFT_WINDOW_SIZE_BY2 - 1; f++) {
    dataSum += data[f];
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
  int numPeaksToRemove = peaksFound - maxNumPeaks;
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
    FFTPeaksAmp[i] = 0;
  }
  return dataSum;
}

// calculates indexes corresponding to frequency bands
void calculateBreadfrequencyBands() {
  // converting from frequency to index
  for (int i = 0; i < NUM_FREQ_BANDS i++) {
    slicerIndexes[i] = ceil(frequencyBands[i] * frequencyWidth);
  }
  // the additional index is the size of 
  slicerIndexes[NUM_FREQ_BANDS] = FFT_WINDOW_SIZE_BY2;
}

// slices fft data into bands, storing the maximum peak in the band and sum of the total energy in the band
void breadslicer(float* data, int size) {
  int sliceCount = 0;
  int sliceIdx = slicerIndexes[sliceCount];
  int sliceSum = 0;
  int sliceMax = 0;
  int sliceMaxIdx = 0;
  for (int f = 0; f < size; f++) {
    // break if exceeding size of data array
    if (f == size - 1) {
      breadslicerSums[sliceCount] = sliceSum + data[f];
      breadslicerPeaks[sliceCount] = sliceMaxIdx;
      break; 
    }
    // get the sum and max amplitude of the current slice
    if (f < sliceIdx) {
      if (data[f] == 0) { continue; }
      sliceSum += data[f];
      if (f == 0 || f == size - 1) { continue; }
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
      if (sliceCount > NUM_FREQ_BANDS) { break; }
      sliceIdx = slicerIndexes[sliceCount];
      f -= 1;
    }
  }
}

// interpolation based on the weight of amplitudes around a peak
int interpolateAroundPeak(float *data, int dataSize, int indexOfPeak, float resolution) {
  float prePeak = indexOfPeak == 0 ? 0 : data[indexOfPeak - 1];
  float atPeak = data[indexOfPeak];
  float postPeak = indexOfPeak == dataSize ? 0 : data[indexOfPeak + 1];
  // summing around the index of maximum amplitude to normalize magnitudeOfChange
  float peakSum = prePeak + atPeak + postPeak;
  // interpolating the direction and magnitude of change, and normalizing from -1.0 to 1.0
  float magnitudeOfChange = ((atPeak + postPeak) - (atPeak + prePeak)) / peakSum;
  
  // return interpolated frequency
  return int((float(indexOfPeak) + magnitudeOfChange) * resolution);
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
      // calculate the sine wave index of the sine wave corresponding to the frequency
      if (sin_wave_amp[s] > 0) {
        float sin_wave_freq_idx = sin_wave_idx * sin_waves_freq[s] * _SAMPLING_FREQ;
        //int sin_wave_position = (sin_wave_freq_idx) % int(SAMPLING_FREQ);
        int sin_wave_position = (sin_wave_freq_idx - floor(sin_wave_freq_idx)) * SAMPLING_FREQ;
        sumOfSines += sin_wave_amp[s] * sin_wave[sin_wave_position];
      }
    }
    // modulate sum of sines by cos wave at this moment in time
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
