/*/
########################################################

  READ ME FIRST!
    This code uses the arduinoFFT library and the Mozzi library. Before running this code, you have to replace all instances of "double" with "float"
    in the arduinoFFT .cpp and .h sourcecode files. Using floating points instead of doubles is more suitable for the ESP32, and helps boost performance.
    
    The sourcecode is located in your libraries folder, usually under: "C:\Users\(YOUR NAME)\Documents\Arduino\libraries\arduinoFFT\src"

  DETAILS REGARDING THE CODE:

    This code uses the Mozzi library for audio synthesis, and it needs to have control over when certain processes are done, to consitently call
  updateAudio() @AUDIO_RATE times per second, which is done to fill the circular audio output buffer. For now, to fill the audio input buffer which is
  the size of @FFT_WIN_SIZE) this is using mozziAnalogRead() - similar to analogRead() but synced with Mozzi library. To do this without causing any
  timing issues and/or major audio glitches, all of audio input, fft analysis, signal processing, as well as modifications to the frequencies and amplitudes
  to synthesize, are completed in updateControl() - a function called by Mozzi @CONTROL_RATE times per second. We need to ensure that all of these functions
  execution time is within @UPDATE_TIME = (1000000 / @CONTROL_RATE) microseconds to ensure that updateAudio() and updateControl() calls are consistent.

  To ensure this, each updateControl() call is dedicated to a certain process that are divided into three phases. 
    1. The audio sampling phase - done however many times needed to fill the audio input buffer recordSample(int), where int is how many samples to record
        per updateControl() call, calculated by dividing @UPDATE_TIME by (@FFT_WINDOW_SIZE * @sampleDelayTime). Moves onto the next process when input
        buffer is full.
    2. Phase A of FFT - calls the most process intensive functions of FFT, which is the Windowing() function, but DCRemoval() is called before Windowing()
        since it needs to be done before and execution time of DCRemoval() is very low (around 30 microseconds). Moves onto the next process when input
        buffer is full.
    3. Phase B of FFT and signal processing - calls the remaining FFT functions FFTCompute() and ComplexToMagnitude(). We're also dedicating this phase to
        signal processing involving the output generated by the previous phases such as the Breadslicer(). 
    4. Along with all these phases and for every updateControl() call, the smoothTranstion() function is also called, this is a simple linear or curve based
        smoothing algorithm that uses pre-calculated values to smoothly transition amplitudes between the previous and next amplitudes this helps reduce audio
        glitches regarding sudden amplitude transitions, the number of smoothing steps is (@CONTROL_RATE / @SAMPLES_PER_SEC). The frequencies of the sine waves
        are changed whenever the fade counter is 0, for the set of waves that have an amplitude of 0.

########################################################
/*/

#include <arduinoFFT.h>
#include <Arduino.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>
#include <EventDelay.h>

/*/
########################################################
    Directives
########################################################
/*/

#define AUDIO_INPUT_PIN_LEFT A2   // left channel audio input
#define AUDIO_INPUT_PIN_RIGHT A3  // right channel audio input

#define SAMPLES_PER_SEC 32  // The number of sampling/analysis cycles per second, this MUST be a power of 2

#define FFT_SAMPLING_FREQ 10000 // The sampling frequency, ideally should be a power of 2
#define FFT_WINDOW_SIZE int(pow(2, 12 - log(SAMPLES_PER_SEC) / log(2))) // the FFT window size and number of audio samples for ideal performance, this MUST be a power of 2 \
                                                                        // 2^(12 - log_base2(SAMPLES_PER_SEC)) = 64 for 64/s, 128 for 32/s, 256 for 16/s, 512 for 8/s

#define CONTROL_RATE int(pow(2, 5 + log(SAMPLES_PER_SEC) / log(2))) // Update control cycles per second for ideal performance, this MUST be a power of 2
                                                                    // 2^(5 + log_base2(SAMPLES_PER_SEC)) = 2048 for 64/s, 1024 for 32/s, 512 for 16/s, 256 for 8/s

#define DEFAULT_NUM_WAVES 6            // The number of waves to synthesize, and how many slices the breadslicer function does
#define OSCIL_COUNT int(int(DEFAULT_NUM_WAVES) << 1)  // The total number of waves to synthesize, DEFAULT_NUM_WAVES * 2

#define BREADSLICER_CURVE_EXPONENT 3.3  // The exponent for the used for the breadslicer curve
#define BREADSLICER_CURVE_OFFSET 0.55 // The curve offset for the breadslicer to follow when slicing the amplitude array

#define BREADSLICER_MAX_AVG_BIN 2000 // The minimum max that is used for scaling the amplitudes to the 0-255 range, and helps represent the volume
#define FFT_FLOOR_THRESH 300                // amplitude flooring threshold for FFT, to help reduce input noise

/*/
########################################################
    FFT
########################################################
/*/

const uint16_t FFT_WIN_SIZE = int(FFT_WINDOW_SIZE);   // the windowing function size of FFT
const float SAMPLE_FREQ = float(FFT_SAMPLING_FREQ);   // the sampling frequency of FFT

// FFT_SIZE_BY_2 is FFT_WIN_SIZE / 2. We are sampling at double the frequency we are trying to detect, therefore only half of the vReal array is used for analysis post FFT
const int FFT_WIN_SIZE_BY2 = int(FFT_WIN_SIZE) >> 1;

const float samplingFreqBy2 = SAMPLE_FREQ / 2.0;  // the frequency we are sampling at divided by 2, since we need to sample twice the frequency we are trying to detect
const float frequencyResolution = float(samplingFreqBy2 / FFT_WIN_SIZE_BY2);  // the frequency resolution of FFT with the current window size

// Number of microseconds to wait between recording audio samples
const int sampleDelayTime = 1000000 / SAMPLE_FREQ;
// The total number of microseconds needed to record enough samples for FFT
const int totalSampleDelay = sampleDelayTime * FFT_WIN_SIZE;

int numSamplesTaken = 0;    // stores the number of audio samples taken, used as iterator to fill audio sample buffer

float vRealL[FFT_WIN_SIZE];  // vRealL is used for input from the left channel and receives computed results from FFT
float vImagL[FFT_WIN_SIZE];  // vImagL is used to store imaginary values for computation

float vRealPrev[FFT_WIN_SIZE_BY2];
float maxAmpChange = 0;
float maxAmpChangeAmp = 0;
int freqMaxAmpChange = 0;

const float OUTLIER = 30000.0;  // Outlier for unusually high amplitudes in FFT

arduinoFFT FFT = arduinoFFT(vRealL, vImagL, FFT_WIN_SIZE, SAMPLE_FREQ);  // Object for performing FFT's

/*/
########################################################
    UPDATE RATE
########################################################
/*/
const int UPDATE_TIME = 1000000 / CONTROL_RATE;                                   // The estimated time in microseconds of each updateControl() call

int nextProcess = 0;                                                              // The next signal aquisition/processing phase to be completed in updateControl
const int numSamplesPerProcess = floor(UPDATE_TIME / sampleDelayTime) - 1;            // The Number of samples to take per update
const int numProcessForSampling = ceil(FFT_WIN_SIZE / float(numSamplesPerProcess));            // the total number of processes to sample, calculated in setup
const int numTotalProcesses = numProcessForSampling + 2;                                // adding 2 more processes for FFT windowing function and other processing

unsigned long sampleT;               // stores the time since program started using mozziMicros() rather than micros()

const int SAMPLE_TIME = int(CONTROL_RATE / SAMPLES_PER_SEC);  // sampling and processing time in updateControl
const int FADE_RATE = SAMPLE_TIME;                            // the number of cycles for fading
const float FADE_STEPX = float(2.0 / (FADE_RATE - 1));        // the x position of the smoothing curve

float FADE_CONST[FADE_RATE];  // Array storing amplitude smoothing constants from 0.0 to 1.0
int fadeCounter = 0;          // counter used to smoothen transition between amplitudes

/*/
########################################################
    Stuff relevant to calculate frequency peaks of an FFT spectrogram
########################################################
/*/

int peakIndexes[FFT_WIN_SIZE_BY2];
int peakAmplitudes[FFT_WIN_SIZE_BY2];

int numWaves = int(DEFAULT_NUM_WAVES);
int sumOfPeakAmps = 0;

/*/
########################################################
    Stuff relevant to breadslicer function
########################################################
/*/

const int slices = numWaves;  // accounts for how many slices the samples are split into
const int totalNumWaves = int(DEFAULT_NUM_WAVES) << 1;  // we are synthesizing twice the number of frequencies we are finding, toggling between the two sets, changing the frequency when the amplitudes of one of the sets are 0

float amplitudeToRange = 254.0 / BREADSLICER_MAX_AVG_BIN / numWaves;   // the "K" value used to put the average amplitudes calculated by breadslicer into the 0-255 range. This value alters but doesn't go below BREADSLICER_MAX_AVG_BIN to give a sense of the volume

int breadslicerSliceLocations[DEFAULT_NUM_WAVES];  // array storing pre-calculated array locations for slicing the FFT amplitudes array (vReal)
const int breadslicerSliceLocationsStatic[DEFAULT_NUM_WAVES] = {4, 8, 15, 25, 50, 64};
float breadslicerSliceWeights[DEFAULT_NUM_WAVES]; // weights associated to the average amplitudes of slices
const float breadslicerSliceWeightsStatic[DEFAULT_NUM_WAVES] = {1.0, 0.6, 0.5, 3.0, 6.0, 8.0}; // weights associated to the average amplitudes of slices

long averageAmplitudeOfSlice[DEFAULT_NUM_WAVES];          // the array used to store the average amplitudes calculated by the breadslicer
int peakFrequencyOfSlice[DEFAULT_NUM_WAVES];              // the array containing the peak frequency of the slice

/*/
########################################################
    Stuff relavent to additive synthesizer
########################################################
/*/

Oscil<2048, AUDIO_RATE> aSinL[OSCIL_COUNT];
int aSinLFrequencies[OSCIL_COUNT];      // the frequencies used for audio synthesis
//Oscil<2048, AUDIO_RATE> aSinR[OSCIL_COUNT];

int amplitudeGains[OSCIL_COUNT];        // the amplitudes used for audio synthesis
int prevAmplitudeGains[OSCIL_COUNT];    // the previous amplitudes, used for smoothing transition
int nextAmplitudeGains[OSCIL_COUNT];    // the next amplitudes
float ampGainStep[OSCIL_COUNT];         // the difference between amplitudes transitions

int prevWaveFrequencies[OSCIL_COUNT];
int nextWaveFrequencies[OSCIL_COUNT];
float freqStep[OSCIL_COUNT];

int toggleFreqChangeWaves = 0;  // toggle frequency transtions between next available waves and the previous waves, to change the frequencies of the next available waves (ones that are at 0 amplitude)

int updateCount = 0;       // updateControl() cycle counter
int audioSamplingSessionsCount = 0;  // counter for the sample and processing phases complete in a full CONTROL_RATE cycle, reset when updateCount is 0

/*/
########################################################
    Setup
########################################################
/*/

/*
  setup() acts as a run once function, do any one time calculations or array initialization here
*/
void setup() {
  // set baud rate
  Serial.begin(115200);
  // wait for Serial to set up
  while (!Serial)
    ; 
  Serial.println("Ready");
  delay(1000);  // wait for one second, not needed but helps prevent weird Serial output
  //Serial.println(breadslicerSliceLocationsStatic);
  // setup reference tables for aSin[] oscillator array and initialize arrays
  for (int i = 0; i < int(OSCIL_COUNT); i++) {
    averageAmplitudeOfSlice[i] = 0;
    aSinL[i].setTable(SIN2048_DATA);
    aSinLFrequencies[i] = int(map(breadslicerSliceLocationsStatic[(i % numWaves)], 1, FFT_WIN_SIZE_BY2, 10, 160));
    amplitudeGains[i] = 0;
    prevAmplitudeGains[i] = 0;
    nextAmplitudeGains[i] = 0;
    prevWaveFrequencies[i] = aSinLFrequencies[i];
    nextWaveFrequencies[i] = aSinLFrequencies[i];
    ampGainStep[i] = 0.0;
    freqStep[i] = 0.0;
  }
  for (int i = 0; i < int(FFT_WIN_SIZE_BY2); i++) {
    vRealPrev[i] = 0.0;
    peakAmplitudes[i] = -1;
  }

  // setup audio input pins
  pinMode(AUDIO_INPUT_PIN_LEFT, INPUT);
  pinMode(AUDIO_INPUT_PIN_RIGHT, INPUT);

  Serial.printf("FFT ANALYSIS PER SECOND: %d\tFFT WINDOW SIZE: %d\tFFT SAMPLING FREQUENCY: %d\tMOZZI CONTROL RATE: %d\tAUDIO RATE: %d\n", SAMPLES_PER_SEC, FFT_WIN_SIZE, FFT_SAMPLING_FREQ, CONTROL_RATE, AUDIO_RATE);
  Serial.println();
  // precalculate linear amplitude smoothing values and breadslicer slice locations to save processing power
  calculateFadeFunction();
  //calculateBreadslicerLocations();
  saveBreadslicerLocations();
  saveBreadslicerWeights();
  
  // Additive Synthesis
  startMozzi(CONTROL_RATE);
  changeFrequencies();
}

/*/
########################################################
    Loop
########################################################
/*/

/*
  audioHook() should be the only function running in loop(), all processor heavy operations should be done in updateControl(), and audio synthesis in updateAudio()
*/
void loop() {
  audioHook();
}

/*/
########################################################
    Additive Synthesis Functions
########################################################
/*/

/*
  updateControl() is called every @CONTROL_RATE times per second, meaning that all functions that are called here have to be completed within a 
  certain period of time that can be calculated by @UPDATE_TIME
*/
void updateControl() {
  // reset update counter and audio sampling sessions counter
  if (updateCount >= CONTROL_RATE) {
    updateCount = 0;
    audioSamplingSessionsCount = 0;
  }
  // get the next available sample time in updateCount
  int sampleTime = SAMPLE_TIME * audioSamplingSessionsCount;
  // if it is time for next audio sample and analysis, begin the audio sampling phase, by setting next process 0 and reset fade counter
  if ((updateCount == 0 || updateCount == sampleTime - 1) && updateCount != CONTROL_RATE - 1) { nextProcess = 0; }

  // audio sampling phase
  if (nextProcess < numProcessForSampling) { recordSample(); }
  // FFT phase A, seperated into two phases so that all functions are ran within UPDATE_TIME
  else if (nextProcess == numTotalProcesses - 2) {
    FFT.DCRemoval();  // Remove DC component of signal
    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
  }
  /* FFT phase B and signal processing phase */
  else if (nextProcess == numTotalProcesses - 1) {
    FFT.Compute(FFT_FORWARD);   // Compute FFT
    FFT.ComplexToMagnitude();   // Compute frequency magnitudes

    // Use the breadslicer function to process FFT output
    breadslicer(vRealL);
    // Frequency of max amplitude change
    //frequencyMaxAmplitudeDelta();
    // Major peaks of an FFT spectrogram
    //findMajorPeaks();

    // mapping amplitudes and frequencies generated by FFT processing to a sine wave frequency and amplitude for additive synthesis
    mapFreqAmplitudes2();
  }
  // called every updateControl tick, to smoothen transtion between amplitudes and change the sine wave frequencies that are used for synthesis
  smoothenTransition();
  // increment updateControl calls and nextProcess counters
  nextProcess++;
  updateCount++;
}

/*
  updateAudio() is called @AUDIO_RATE times per second, the code in here needs to be very basic or audio glitches will occur
*/
AudioOutput_t updateAudio() {
  long longCarrierLeft = 0;    // stores the carrier wave for the left channel
  //int longCarrierRight = 0; // stores the carrier wave for the right channel

  for (int i = 0; i < numWaves; i++) {
    // summing sine waves into a "long" carrier wave
    longCarrierLeft += amplitudeGains[i] * aSinL[i].next();
    //longCarrierRight += amplitudeGains[i] * aSinR[i].next();
  }
  // bit shift down to appropriate range (0-255)
  return MonoOutput::from8Bit(int8_t(longCarrierLeft >> 8));
  //return StereoOutput::from8Bit(int8_t(longCarrierLeft >> 8), int8_t(longCarrierRight >> 8));
}

/*/
########################################################
    Functions related to FFT and FFT analysis
########################################################
/*/

void saveBreadslicerLocations() {
  int lastSliceFrequency = 0;
  for (int i = 0; i < slices; i++) {
    // store location in array that is used by the breadslicer function
    int sliceLocation = breadslicerSliceLocationsStatic[i];
    breadslicerSliceLocations[i] = sliceLocation;
    int sliceFrequency = sliceLocation * frequencyResolution;
    Serial.printf("\t\tsliceLocation %d, Range %dHz to %dHz\n", sliceLocation, lastSliceFrequency, sliceFrequency);
    lastSliceFrequency = sliceFrequency;
  }
  Serial.println();
}

void saveBreadslicerWeights() {
  for (int i = 0; i < slices; i++) {
    breadslicerSliceWeights[i] = breadslicerSliceWeightsStatic[i];
  }
}

/*
  Used to pre-calculate the array locations for the breadslicer to reduce processor load during loop
*/
void calculateBreadslicerLocations() {
  // The curve to follow for binning the amplitudes array: x^(1 / ((x^BREADSLICER_CURVE_EXPONENT) + BREADSLICER_CURVE_OFFSET)) where x is between 0.0 and 1.0
  float step = 1.0 / slices;       // how often to step on the x-axis to determine bin value
  // calculate array locations for the breadslicer
  Serial.println("Calculating array slice locations for breadslicer...");
  Serial.printf("\tUsing curve: x ^ (1 / ((x ^ %0.3f) + %0.3f)), where X is from 0.0 to 1.0\n", BREADSLICER_CURVE_EXPONENT, BREADSLICER_CURVE_OFFSET);
  // stores the previous slices frequency, only used for printing
  int lastSliceFrequency = 0;
  for (int i = 0; i < slices; i++) {
    float xStep = (i + 1) * step;        // x-axis step (0.0 - 1.0)
    //float exponent = 1.0 / (pow(xStep, BREADSLICER_CURVE_EXPONENT) + BREADSLICER_CURVE_OFFSET);  // exponent of the curve
    float exponent = 1.0 / 0.5;
    // calculate slice location in array based on the curve to follow for slicing.
    int sliceLocation = round(FFT_WIN_SIZE_BY2 * pow(xStep, exponent));
    int sliceFrequency = round(sliceLocation * frequencyResolution);      // Calculates the slices frequency range
    // The breadslicer function uses less-than sliceLocation in its inner loop, which is why we add one.
    if (sliceLocation < FFT_WIN_SIZE_BY2) {
      sliceLocation += 1;
    }
    // store location in array that is used by the breadslicer function
    breadslicerSliceLocations[i] = sliceLocation;
    // print the slice locations and it's associated frequency
    Serial.printf("\t\tsliceLocation %d, Range %dHz to %dHz\n", sliceLocation, lastSliceFrequency, sliceFrequency);
    // store the previous slices frequency
    lastSliceFrequency = sliceFrequency;
  }
  Serial.println("\n");
}

/*
  Splits the amplitude data (vReal array) into X bands based on somewhat of a parabolic/logarithmic curve, where X is sliceInto. This is done to detect features of sound which are commonly found at certain frequency ranges.
  For example, 0-200Hz is where bass is common, 200-1200Hz is where voice and instruments are common, 1000-5000Hz higher notes and tones as well as harmonics 
*/
void breadslicer(float *data) {
  // The maximum amplitude of the array. Set to BREADSLICER_MAX_AVG_BIN to scale all amplitudes
  // to the 0-255 range and to represent volume
  long curMaxAvg = BREADSLICER_MAX_AVG_BIN;

  float topOfSample = frequencyResolution;  // The frequency of the current amplitude in the amplitudes array

  // lastJ is the last amplitude taken from *data. It is set to 1 to skip the first amplitude at 0Hz
  int lastJ = 1;
  // Calculate the size of each bin and amplitudes per bin
  for (int i = 0; i < slices; i++) {
    // use pre-calculated array locations for amplitude array slicing
    int sliceSize = breadslicerSliceLocations[i];

    // store the array location pre-inner loop
    int newJ = lastJ;

    long ampSliceSum = 0;     // the sum of the current slice
    int ampSliceCount = 0;   // the number of amplitudes in the current slice
    // these values are used to determine the "peak" of the current slice
    int maxSliceAmp = 0;
    int maxSliceAmpFreq = 0;

    // inner loop, finds the average energy of the slice and "peak" frequency
    for (int j = lastJ; j < sliceSize; j++) {
      topOfSample += frequencyResolution;  // calculate the associated frequency
      // store data
      int amp = round(data[j]);
      // if amplitude is above certain threshold and below outlier value, then exclude from signal processing
      if (amp > FFT_FLOOR_THRESH && amp < OUTLIER) {
        // add to the slice sum, and average frequency, increment amps above threshold counter
        ampSliceSum += amp;
        // the peak amplitude of the current slice, this is not exactly the "peak" of the spectogram since slicing the array can cut off peaks. But it is good enough for now.
        if (amp > maxSliceAmp) {
          maxSliceAmp = amp;
          maxSliceAmpFreq = int(round(topOfSample));
        }
      }
      // increment the current slice's amplitude count
      ampSliceCount += 1;
      // save last location in array
      newJ += 1;

      // Debugging stuff
      // Serial.printf("I: %d\tFreq: %.2f\tAmp: %.2f\t", j, topOfSample, data[j]);
      // Serial.println();
    }
    //ampSliceSum *= frequencyResolution;
    //if the current slice contains at least two amplitudes, then assign the average, otherwise assign the sum of the slice
    if (ampSliceCount > 1) {
      averageAmplitudeOfSlice[i] = int(round(ampSliceSum / ampSliceCount));
    } else {
      averageAmplitudeOfSlice[i] = round(ampSliceSum);
    }
    averageAmplitudeOfSlice[i] = round(averageAmplitudeOfSlice[i] * breadslicerSliceWeights[i]);
    // if there is at least one amplitude that is above threshold in the group, map it's frequency, otherwise frequency for that slice is unchanged
    if (ampSliceCount > 0) {
      peakFrequencyOfSlice[i] = maxSliceAmpFreq;
    }
    // ensure that averages aren't higher than the max set average value, and if it is higher, then the max average is replaced by that value, for this iteration
    if (averageAmplitudeOfSlice[i] > curMaxAvg) {
        curMaxAvg = averageAmplitudeOfSlice[i];
    }
    // set the iterator to the next location in the array for the next slice
    lastJ = newJ;
  }
  // put all amplitudes within the 0-255 range, with this "K" value, done in mapFreqAmplitudes()
  amplitudeToRange = float(254.0 / curMaxAvg / numWaves);
}

/* 
  Find all the major peaks in the fft spectrogram, based on the threshold and how many peaks to find 
*/
void findMajorPeaks(float *data) {
  int majorPeaksCounter = 0;
  int peakReached = 0;
  // traverse through spectrogram and find all peaks (above FFT_FLOOR_THRESH)
  for (int i = 1; i < FFT_WIN_SIZE_BY2; i++) {
    // skip the iteration if the fft amp is an unsually high value to exclude them from calculations (usually occurs on the lowest and highest frequencies)
    if (data[i - 1] > OUTLIER) {
      continue;
    }
    // get the change between the next and last index
    float change = data[i] - data[i - 1];
    // if change is positive, consider that the peak hasn't been reached yet
    if (change > 0.0) {
      peakReached = 0;
      // otherwise if change is 0 or negative, consider this the peak
    } else {
      // waiting for next positive change, to start looking for peak
      if (peakReached == 0) {
        // if the amplitude is higher then a certain threshold, consider a peak has been reached
        if (data[i - 1] > FFT_FLOOR_THRESH) {
          // print all considered peaks so far
          //Serial.printf("(%d,%d),", i - 1, int(round(data[i - 1])));
          peakReached = 1;

          /* array buffering based on the number peak frequencies being returned */
          // if the array is not 'full', iterate peak counter and store peak into array (about 20 lines down)
          if (majorPeaksCounter < numWaves) {
            majorPeaksCounter++;
          } else {  // otherwise, if array is 'full',
            // set the lowest peak index to the last value in the array
            int lowestPeakIndex = majorPeaksCounter - 1;
            // find the lowest amplitude in the peak amplitudes array
            for (int j = 0; j < numWaves - 1; j++) {
              if (peakAmplitudes[j] < data[i - 1]) {
                lowestPeakIndex = j;
              }
            }
            // if the lowest peak is the lastPeak then check if the last lowest peak was lower
            if (lowestPeakIndex == majorPeaksCounter - 1) {
              if (peakAmplitudes[lowestPeakIndex] < data[i - 1]) {
                peakAmplitudes[lowestPeakIndex] = data[i - 1];
                peakIndexes[lowestPeakIndex] = i - 1;
              }
              continue;
            }
            // based on the lowest amplitude index, scoot the array to the right to remove the lowest peak, and store the highest peak
            for (int j = lowestPeakIndex; j < numWaves - 1; j++) {
              peakAmplitudes[j] = peakAmplitudes[j + 1];
              peakIndexes[j] = peakIndexes[j + 1];
            }
          }
          peakIndexes[majorPeaksCounter - 1] = i - 1;
          peakAmplitudes[majorPeaksCounter - 1] = int(round(data[i - 1]));
        }
      }
    }
  }
  // set the remaining peaks to -1, to exclude them from further processing
  for (int j = majorPeaksCounter; j < numWaves; j++) {
    peakAmplitudes[j] = -1;
    //peakIndexes[j] = -1;
  }
  //Serial.println();
}

/* 
  Finds the frequency with the most dominant change in amplitude, by comparing 2 consecutive FFT amplitude arrays 
*/
void frequencyMaxAmplitudeDelta(float *data, float *prevData) {
  maxAmpChange = 0;
  int maxAmpChangeI = 0;
  for (int i = 0; i < FFT_WIN_SIZE_BY2; i++) {
    if (data[i] < OUTLIER) {
      int currAmpChange = abs(data[i] - prevData[i]);
      if (currAmpChange > maxAmpChange) {
        maxAmpChange = currAmpChange;
        maxAmpChangeI = i;
      }
    }
    prevData[i] = data[i];
  }
  peakAmplitudes[numWaves] = map(data[maxAmpChangeI], 0, sumOfPeakAmps, 0, 255);

  freqMaxAmpChange = int(round((maxAmpChangeI + 1) * frequencyResolution));
  //targetWaveFrequencies[numWaves] = map(freqMaxAmpChange, 0, int(samplingFreqBy2), 16, 200);
}

/* 
  Records samples into the sample buffer array by reading from the audio input pins, at a sampling frequency of SAMPLE_FREQ
*/
void recordSample() {
  for (int i = 0; i < numSamplesPerProcess; i++) {
    sampleT = micros();
    int sampleL = mozziAnalogRead(AUDIO_INPUT_PIN_LEFT);    // Read sample from left input
    vRealL[numSamplesTaken] = float(sampleL);       // store left channel sample in left vReal array
    vImagL[numSamplesTaken] = 0.0;                  // set imaginary values to 0.0
    numSamplesTaken++;                              // increment sample counter
    // break out of loop if buffer is full, reset sample counter and increment the total number of sampling sessions
    if (numSamplesTaken == FFT_WIN_SIZE) {
      numSamplesTaken = 0;
      audioSamplingSessionsCount++;
      break;
    }
    if (i == numSamplesPerProcess - 1) { break; }
    // wait until sampleDelayTime has passed, to sample at the sampling frequency. Without this the DCRemoval() function will not work.
    while (micros() - sampleT < sampleDelayTime);
  }
}

/*/
########################################################
    Functions related to mapping amplitudes and frequencies generated by FFT analysis functions
########################################################
/*/

/*
  Used to precalculate the values for the fade function to reduce processor load during loop
*/
void calculateFadeFunction() {
  Serial.println("Pre-calculating fade function constants...");
  for (int i = 0; i < FADE_RATE; i++) {
    float xStep = i * FADE_STEPX - 1.0;
    if (xStep < 0.0) {
      FADE_CONST[i] = -(pow(abs(xStep), 1.0) * 0.5) + 0.5;
    } else {
      FADE_CONST[i] = pow(xStep, 1.0) * 0.5 + 0.5;
    }
    Serial.printf("\tfadeCounter: %d, xValue: %0.2f, yValue: %.2f\n", i, xStep, FADE_CONST[i]);
  }
  Serial.println();
}


/* 
  Maps the amplitudes and frequencies just generated by FFT to the next amplitudes to be synthesized, which is done to gradually increase/decrease
  the real amplitudes to the next amplitudes with linear smoothing (can be changed to curve smoothing). 
*/
void mapFreqAmplitudes2() {
  // for the total number of waves (which is 2 * numWaves), assign the frequency and the amplitudes
  // just generated by FFT to the amplitudes and frequencies of sine waves that will be synthesized
  for (int i = 0; i < numWaves; i++) {
    // kind of like a circular buffer to smoothen transition between amplitudes to reduce audio glitches,
    // results in a delay between input and output of about (1/SAMPLES_PER_SEC) seconds
    prevAmplitudeGains[i] = nextAmplitudeGains[i];
    prevWaveFrequencies[i] = nextWaveFrequencies[i];
    int gainValue = 0;    // the gain values to assign to the next wave being synthesized
    int frequency = 0;    // the frequency to assign to the next wave being synthesized
    // if iterator is on one of the next available waves (waves with 0 gain), map its amplitude, otherwise gain value is 0
    // use the breadslicer output for mapping the amplitudes and frequencies
    gainValue = averageAmplitudeOfSlice[i];
    // only map the gain and frequency if the gain value of the current slice is above 0 meaning no meaningful energy was detected in that slice
    if (gainValue > 0) {
      frequency = peakFrequencyOfSlice[i];
      // multiply the gain value by a "K" value to put it into the 0-255 range, this value alters based on the max average amplitude but doesn't go below a certain point
      gainValue = round(gainValue * amplitudeToRange);
      // slightly shrink down the bassy frequencys
      if (frequency < 234) {
        frequency = round(frequency * 0.4);
      // shrink the rest of the peak frequencies by mapping them to them to a range
      } else {
        // normalize the frequency to a value between 0.0 and 1.0
        float normalizedFreq = (frequency - 234.0) / (samplingFreqBy2 - 234.0);
        // this is done to do a logarithmic mapping rather than linear
        normalizedFreq = pow(normalizedFreq, 0.6);
        frequency = int(round(normalizedFreq * 130) + 30);
      }
    }
    nextAmplitudeGains[i] = gainValue;
    nextWaveFrequencies[i] = frequency;
    // assign the gain value to the nextAmplitudeGains[i], the next waves to be synthesized
    // calculate the value to alter the amplitude per each update cycle
    ampGainStep[i] = float(nextAmplitudeGains[i] - prevAmplitudeGains[i]);
    freqStep[i] = float(nextWaveFrequencies[i] - prevWaveFrequencies[i]);
    // Serial.printf("(%d, %d, %03d, %03d)\t", i, i - aLocation, gainValue, frequency);
  }
  // Serial.println();
}


/* 
  Maps the amplitudes and frequencies just generated by FFT to the next amplitudes to be synthesized, which is done to gradually increase/decrease
  the real amplitudes to the next amplitudes with linear smoothing (can be changed to curve smoothing). 
*/
void mapFreqAmplitudes() {
  // aLocation accounts for array locations of the waves whose amplitudes are at 0, so that their frequencies can be changed
  int aLocation = 0;
  // aLocationMax accounts for maximum array location of the frequencies whos amplitudes are at 0
  int aLocationMax = numWaves;
  // toggle between the two sets of waves
  if (toggleFreqChangeWaves == 1) {
    aLocation = numWaves;
    aLocationMax = totalNumWaves;
  }
  // for the total number of waves (which is 2 * numWaves), assign the frequency and the amplitudes
  // just generated by FFT to the amplitudes and frequencies of sine waves that will be synthesized
  for (int i = 0; i < totalNumWaves; i++) {
    // kind of like a circular buffer to smoothen transition between amplitudes to reduce audio glitches,
    // results in a delay between input and output of about (1/SAMPLES_PER_SEC) seconds
    prevAmplitudeGains[i] = nextAmplitudeGains[i];
    int gainValue = 0;    // the gain values to assign to the next wave being synthesized
    int frequency = 0;    // the frequency to assign to the next wave being synthesized
    // if iterator is on one of the next available waves (waves with 0 gain), map its amplitude, otherwise gain value is 0
    if (i >= aLocation && i < aLocationMax) {
      // use the breadslicer output for mapping the amplitudes and frequencies
      gainValue = averageAmplitudeOfSlice[i - aLocation];
      // only map the gain and frequency if the gain value of the current slice is above 0 meaning no meaningful energy was detected in that slice
      if (gainValue > 0) {
        frequency = peakFrequencyOfSlice[i - aLocation];
        // multiply the gain value by a "K" value to put it into the 0-255 range, this value alters based on the max average amplitude but doesn't go below a certain point
        gainValue = round(gainValue * amplitudeToRange);
        // slightly shrink down the bassy frequencys
        if (frequency < 240) {
          frequency = round(frequency * 0.3);
        // shrink the rest of the peak frequencies by mapping them to them to a range
        } else {
          // normalize the frequency to a value between 0.0 and 1.0
          float normalizedFreq = (frequency - 240.0) / (samplingFreqBy2 - 240.0);
          // this is done to do a logarithmic mapping rather than linear
          normalizedFreq = pow(normalizedFreq, 0.6);
          frequency = int(round(normalizedFreq * 110) + 50);
        }
        // assign new frequency to an ossilator, no need to smoothen this since the gain value for this ossilator is 0
        aSinLFrequencies[i] = frequency;
      }
      nextAmplitudeGains[i] = gainValue;
    }
    // assign the gain value to the nextAmplitudeGains[i], the next waves to be synthesized
    // calculate the value to alter the amplitude per each update cycle
    ampGainStep[i] = float(nextAmplitudeGains[i] - prevAmplitudeGains[i]);
    // Serial.printf("(%d, %d, %03d, %03d)\t", i, i - aLocation, gainValue, frequency);
  }
  // Serial.println();
  // toggle between waves for which frequencies will be altered
  toggleFreqChangeWaves = 1 - toggleFreqChangeWaves;
}

/*
  changes the frequencies of the set of waves with an amplitude of 0
*/
void changeFrequencies() {
  for (int i = 0; i < numWaves; i++) {
    aSinL[i].setFreq(aSinLFrequencies[i]);
    aSinL[i + numWaves].setFreq(aSinLFrequencies[i + numWaves]);
  }
}

/*
  Linear smoothing between amplitudes to reduce audio glitches, using precalculated values
*/
void smoothenTransition() {
  for (int i = 0; i < numWaves; i++) {
    // amplitudeGains[i] = round(prevAmplitudeGains[i] + FADE_CONST[fadeCounter] * ampGainStep[i]);
    // amplitudeGains[i + numWaves] = round(prevAmplitudeGains[i + numWaves] + FADE_CONST[fadeCounter] * ampGainStep[i + numWaves]);
    // smoothen the transition between each amplitude, checking for difference to avoid additonal smoothing
    if (abs(nextAmplitudeGains[i] - int(round(amplitudeGains[i]))) > 3) {  
      amplitudeGains[i] = round(prevAmplitudeGains[i] + float(FADE_CONST[fadeCounter] * ampGainStep[i]));
    } else {
      amplitudeGains[i] = nextAmplitudeGains[i];
    }
    aSinLFrequencies[i] = round(prevWaveFrequencies[i] + float(FADE_CONST[fadeCounter] * freqStep[i]));
    aSinL[i].setFreq(int(aSinLFrequencies[i]));
  }
  // increment fade counter
<<<<<<< HEAD
  fadeCounter = (fadeCounter + 1) % FADE_RATE;
}
=======
  fadeCounter_1++;
}
>>>>>>> 0d7b984c1e093169408d0ec3ae76dc3099c46dfb
