/**@file Vibrosonics.ino */
/*/
########################################################
  READ ME FIRST! This code uses the arduinoFFT library and the Mozzi library. Before running this code, you have to replace all instances of
  "double" with "float" in the arduinoFFT .cpp and .h sourcecode files. Using floating points instead of doubles is more suitable for the ESP32,
  and helps boost performance. The sourcecode is located in your libraries folder, usually under:
    C:\Users\(YOUR NAME)\Documents\Arduino\libraries\arduinoFFT\src
  WILL ADD MORE INFORMATION SOON...
########################################################
/*/

#include <arduinoFFT.h>
#include <Arduino.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>

/*
########################################################
    Directives
########################################################
*/

#define AUDIO_INPUT_PIN_LEFT A2   ///< left channel audio input
#define AUDIO_INPUT_PIN_RIGHT A3  ///< right channel audio input

#define SAMPLES_PER_SEC 32  ///< The number of sampling/analysis cycles per second, this MUST be a power of 2

#define FFT_SAMPLING_FREQ 8192 ///< The sampling frequency, ideally should be a power of 2
#define FFT_WINDOW_SIZE int(pow(2, 12 - log(SAMPLES_PER_SEC) / log(2))) ///< the FFT window size and number of audio samples for ideal performance, this MUST be a power of 2 \
                                                                        ///< 2^(12 - log_base2(SAMPLES_PER_SEC)) = 64 for 64/s, 128 for 32/s, 256 for 16/s, 512 for 8/s

#define CONTROL_RATE int(pow(2, 5 + log(SAMPLES_PER_SEC) / log(2))) ///< Update control cycles per second for ideal performance, this MUST be a power of 2
                                                                    ///< 2^(5 + log_base2(SAMPLES_PER_SEC)) = 2048 for 64/s, 1024 for 32/s, 512 for 16/s, 256 for 8/s

#define DEFAULT_NUM_WAVES 5            ///< The number of waves to synthesize, and how many slices the breadslicer function does
#define OSCIL_COUNT int(DEFAULT_NUM_WAVES) << 1  ///< The total number of waves to synthesize, DEFAULT_NUM_WAVES * 2

#define BREADSLICER_CURVE_EXPONENT 3.0  ///< The exponent for the used for the breadslicer curve
#define BREADSLICER_CURVE_OFFSET 0.59  ///< The curve offset for the breadslicer to follow when slicing the amplitude array

#define DEFAULT_BREADSLICER_MAX_AVG_BIN 2000 ///< The minimum max that is used for scaling the amplitudes to the 0-255 range, and helps represent the volume
#define FFT_FLOOR_THRESH 300                ///< amplitude flooring threshold for FFT, to help reduce input noise

/*
########################################################
    FFT
########################################################
*/

const uint16_t FFT_WIN_SIZE = int(FFT_WINDOW_SIZE);   ///< the windowing function size of FFT
const float SAMPLE_FREQ = float(FFT_SAMPLING_FREQ);   ///< the sampling frequency of FFT

///< FFT_SIZE_BY_2 is FFT_WIN_SIZE / 2. We are sampling at double the frequency we are trying to detect, therefore only half of the vReal array is used for analysis post FFT
const int FFT_WIN_SIZE_BY2 = int(FFT_WIN_SIZE) >> 1;

const float samplingFreqBy2 = SAMPLE_FREQ / 2.0;  ///< the frequency we are sampling at divided by 2, since we need to sample twice the frequency we are trying to detect
const float frequencyResolution = float(samplingFreqBy2 / FFT_WIN_SIZE_BY2);  ///< the frequency resolution of FFT with the current window size

// Number of microseconds to wait between recording audio samples
const int sampleDelayTime = 1000000 / SAMPLE_FREQ;
// The total number of microseconds needed to record enough samples for FFT
const int totalSampleDelay = sampleDelayTime * FFT_WIN_SIZE;

int numSamplesTaken = 0;    ///< stores the number of audio samples taken, used as iterator to fill audio sample buffer

float vRealL[FFT_WIN_SIZE];  ///< vRealL is used for input from the left channel and receives computed results from FFT
float vImagL[FFT_WIN_SIZE];  ///< vImagL is used to store imaginary values for computation
float vRealR[FFT_WIN_SIZE];  ///< vRealR is used for input from the right channel and receives computed results from FFT
float vImagR[FFT_WIN_SIZE];  ///< vImagR is used to store imaginary values for computation


float vRealPrev[FFT_WIN_SIZE_BY2];
float maxAmpChange = 0;
float maxAmpChangeAmp = 0;
int freqMaxAmpChange = 0;

const float OUTLIER = 30000.0;  ///< Outlier for unusually high amplitudes in FFT

arduinoFFT FFT = arduinoFFT(vRealL, vImagL, FFT_WIN_SIZE, SAMPLE_FREQ);  // Object for performing FFT's

/*
########################################################
    UPDATE RATE
########################################################
*/
const int UPDATE_TIME = 1000000 / CONTROL_RATE;                                   ///< The estimated time in microseconds of each updateControl() call

int nextProcess = 0;                                                              ///< The next signal aquisition/processing phase to be completed in updateControl
const int numSamplesPerProcess = floor(UPDATE_TIME / sampleDelayTime);            ///< The Number of samples to take per update
const int numProcessForSampling = ceil(FFT_WIN_SIZE / numSamplesPerProcess);            ///< the total number of processes to sample, calculated in setup
const int numTotalProcesses = numProcessForSampling + 2;                                ///< adding 2 more processes for FFT windowing function and other processing

volatile unsigned long sampleT;               ///< stores the time since program started using mozziMicros() rather than micros()

const int SAMPLE_TIME = int(CONTROL_RATE / SAMPLES_PER_SEC);  ///< sampling and processing time in updateControl
const int FADE_RATE = SAMPLE_TIME;                            ///< the number of cycles for fading
const float FADE_STEPX = float(2.0 / (FADE_RATE - 1));        ///< the x position of the smoothing curve

float FADE_CONST[FADE_RATE];  ///< Array storing amplitude smoothing constants from 0.0 to 1.0
int fadeCounter = 0;          ///< counter used to smoothen transition between amplitudes

/*
########################################################
    Stuff relevant to calculate frequency peaks of an FFT spectrogram
########################################################
*/

int peakIndexes[FFT_WIN_SIZE_BY2];
int peakAmplitudes[FFT_WIN_SIZE_BY2];

int numWaves = int(DEFAULT_NUM_WAVES);
int sumOfPeakAmps = 0;

/*
########################################################
    Stuff relevant to breadslicer function
########################################################
*/

const int slices = numWaves;  ///< accounts for how many slices the samples are split into
const int totalNumWaves = int(DEFAULT_NUM_WAVES) << 1;  ///< we are synthesizing twice the number of frequencies we are finding, toggling between the two sets, changing the frequency when the amplitudes of one of the sets are 0

float amplitudeToRange = (255.0/DEFAULT_BREADSLICER_MAX_AVG_BIN) / numWaves;   ///< the "K" value used to put the average amplitudes calculated by breadslicer into the 0-255 range. This value alters but doesn't go below DEFAULT_BREADSLICER_MAX_AVG_BIN to give a sense of the volume

int breadslicerSliceLocations[DEFAULT_NUM_WAVES];  ///< array storing pre-calculated array locations for slicing the FFT amplitudes array (vReal)
float breadslicerSliceWeights[DEFAULT_NUM_WAVES] = {0.6, 1.5, 2.5, 2.0, 2.0}; ///< weights associated to the average amplitudes of slices

int averageAmplitudeOfSlice[OSCIL_COUNT];          ///< the array used to store the average amplitudes calculated by the breadslicer
int peakFrequencyOfSlice[OSCIL_COUNT];              ///< the array containing the peak frequency of the slice

/*
########################################################
    Stuff relavent to additive synthesizer
########################################################
*/

Oscil<2048, AUDIO_RATE> aSinL[OSCIL_COUNT];
//Oscil<2048, AUDIO_RATE> aSinR[OSCIL_COUNT];

int amplitudeGains[OSCIL_COUNT];      ///< the amplitudes used for synthesis
int prevAmplitudeGains[OSCIL_COUNT];    ///< the previous amplitudes, used for curve smoothing
int nextAmplitudeGains[OSCIL_COUNT];    ///< the target amplitudes
float ampGainStep[OSCIL_COUNT];         ///< the difference between amplitudes transitions

int toggleFreqChangeWaves = 0;  ///< toggle frequency transtions between next available waves and the previous waves, to change the frequencies of the next available waves (ones that are at 0 amplitude)

int updateCount = 0;       ///< updateControl() cycle counter
int audioSamplingSessionsCount = 0;  ///< counter for the sample and processing phases complete in a full CONTROL_RATE cycle, reset when updateCount is 0

/*
########################################################
  Other
########################################################
*/

int opmode = 0;

/*
########################################################
    Setup
########################################################
*/


/**
 * setup() acts as a run once function, do any one time calculations or array initialization here. Default Arduino function.
 * 
 * 
 * @param N/A
 * @return N/A
 */
void setup() {
  // set baud rate
  Serial.begin(115200);
  // wait for Serial to set up
  while (!Serial)
    ;
  Serial.println("Ready");

  // setup reference tables for aSin[] oscillator array and initialize arrays
  for (int i = 0; i < OSCIL_COUNT; i++) {
    averageAmplitudeOfSlice[i] = 0;
    aSinL[i].setTable(SIN2048_DATA);
    //aSinR[i].setTable(SIN2048_DATA);
    amplitudeGains[i] = 0;
    prevAmplitudeGains[i] = 0;
    nextAmplitudeGains[i] = 0;
    ampGainStep[i] = 0.0;
  }

  // setup audio input pins
  pinMode(AUDIO_INPUT_PIN_LEFT, INPUT);
  pinMode(AUDIO_INPUT_PIN_RIGHT, INPUT);

  // initialize other arrays to 0.0
  for (int i = 0; i < FFT_WIN_SIZE_BY2; i++) {
    vRealPrev[i] = 0.0;
    peakAmplitudes[i] = -1;
  }

  delay(1000);  // wait for one second, not needed but helps prevent weird Serial output

  Serial.printf("FFT ANALYSIS PER SECOND: %d\tFFT WINDOW SIZE: %d\tFFT SAMPLING FREQUENCY: %d\tMOZZI CONTROL RATE: %d\tAUDIO RATE: %d\n", SAMPLES_PER_SEC, FFT_WIN_SIZE, FFT_SAMPLING_FREQ, CONTROL_RATE, AUDIO_RATE);
  Serial.println();
  // precalculate linear amplitude smoothing values and breadslicer slice locations to save processing power
  calculateFadeFunction();
  calculateBreadslicerLocations();
  
  // Additive Synthesis
  startMozzi(CONTROL_RATE);
}

/*
########################################################
    Loop
########################################################
*/


/**
 * audioHook() should be the only function running in loop(), all processor heavy operations should be done in updateControl(), and audio synthesis in updateAudio().
 * audioHook() is the mozzi library wrapper for its loops and output. Default Arduino function.
 * 
 * @param N/A
 * @return N/A
 */

void loop() {
  audioHook();
}

/*
########################################################
    Additive Synthesis Functions
########################################################
*/


/**
 * updateControl() is called every @CONTROL_RATE times per second, meaning that all functions that are called here have to be completed within a 
 * certain period of time that can be calculated by @UPDATE_TIME. Mozzi Library function.
 * 
 * @param N/A
 * @return N/A
 */
void updateControl() {
  // called every updateControl tick, to smoothen transtion between amplitudes and change the sine wave frequencies that are used for synthesis
  smoothenTransition();
  // reset update counter and audio sampling sessions counter
  if (updateCount >= CONTROL_RATE) {
    updateCount = 0;
    audioSamplingSessionsCount = 0;
  }
  // get the next available sample time in updateCount
  int sampleTime = SAMPLE_TIME * audioSamplingSessionsCount;
  // if it is time for next audio sample and analysis, begin the audio sampling phase, by setting next process 0 and reset fade counter
  if ((updateCount == 0 || updateCount == sampleTime - 1) && updateCount != CONTROL_RATE - 1) {
    nextProcess = 0;
    fadeCounter = 0;
  }
  // audio sampling phase
  if (nextProcess < numProcessForSampling) {
    recordSample(numSamplesPerProcess);
  }
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
    breadslicer(vRealL, slices);
    // Frequency of max amplitude change
    //frequencyMaxAmplitudeDelta();
    // Major peaks of an FFT spectrogram
    //findMajorPeaks();

    // mapping amplitudes and frequencies generated by FFT processing to a sine wave frequency and amplitude for additive synthesis
    mapFreqAmplitudes();
  }
  // increment updateControl calls and nextProcess counters
  nextProcess++;
  updateCount++;
}


/**
 * updateAudio() is called @AUDIO_RATE times per second, the code in here needs to be very basic or audio glitches will occur.
 * Mozzi Library function.
 * 
 * @param N/A
 * @return N/A
 */
AudioOutput_t updateAudio() {
  int longCarrierLeft = 0;    // stores the carrier wave for the left channel
  //int longCarrierRight = 0; // stores the carrier wave for the right channel

  for (int i = 0; i < totalNumWaves; i++) {
    // summing sine waves into a "long" carrier wave
    longCarrierLeft += amplitudeGains[i] * aSinL[i].next();
    //longCarrierRight += amplitudeGains[i] * aSinR[i].next();
  }
  // bit shift down to appropriate range (0-255)
  return MonoOutput::from8Bit(int8_t(longCarrierLeft >> 8));
  //return StereoOutput::from8Bit(int8_t(longCarrierLeft >> 8), int8_t(longCarrierRight >> 8));
}

/*
########################################################
    Functions related to FFT and FFT analysis
########################################################
*/


/**
 * Used to pre-calculate the array locations for the breadslicer to reduce processor load during loop
 * 
 * @param N/A
 * @return N/A
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
    float exponent = 1.0 / (pow(xStep, BREADSLICER_CURVE_EXPONENT) + BREADSLICER_CURVE_OFFSET);  // exponent of the curve
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


/**
 * Splits the amplitude data (vReal array) into X bands based on somewhat of a parabolic/logarithmic curve, where X is sliceInto. This is done to detect features of sound which are commonly found at certain frequency ranges.
 * For example, 0-200Hz is where bass is common, 200-1200Hz is where voice and instruments are common, 1000-5000Hz higher notes and tones as well as harmonics 
 * 
 * @param float* data
 * @param int sliceInto
 * @return N/A
 */
void breadslicer(float *data, int sliceInto) {
  // The maximum amplitude of the array. Set to DEFAULT_BREADSLICER_MAX_AVG_BIN to scale all amplitudes
  // to the 0-255 range and to represent volume
  int curMaxAvg = DEFAULT_BREADSLICER_MAX_AVG_BIN;

  float topOfSample = frequencyResolution;  // The frequency of the current amplitude in the amplitudes array

  // lastJ is the last amplitude taken from *data. It is set to 1 to skip the first amplitude at 0Hz
  int lastJ = 1;
  // Calculate the size of each bin and amplitudes per bin
  for (int i = 0; i < sliceInto; i++) {
    // use pre-calculated array locations for amplitude array slicing
    int sliceSize = breadslicerSliceLocations[i];

    // store the array location pre-inner loop
    int newJ = lastJ;

    int ampSliceSum = 0;     // the sum of the current slice
    int ampSliceCount = 0;   // the number of amplitudes in the current slice
    // these values are used to determine the "peak" of the current slice
    int maxSliceAmp = 0;
    int maxSliceAmpFreq = 0;
    // the averageCount, counts how many amplitudes are above the FFT_FLOOR_THRESH, the averageFreq contains the average frequency of these amplitudes
    int ampsAboveThresh = 0;
    int averageFreq = 0;

    // inner loop, finds the average energy of the slice and "peak" frequency
    for (int j = lastJ; j < sliceSize; j++) {
      topOfSample += frequencyResolution;  // calculate the associated frequency
      // store data
      int amp = round(data[j]);
      // if amplitude is above certain threshold and below outlier value, then exclude from signal processing
      if (amp > FFT_FLOOR_THRESH && amp < OUTLIER) {
        // floor the current amplitude, considered 0 otherwise since its excluded
        //amp -= FFT_FLOOR_THRESH;
        // add to the slice sum, and average frequency, increment amps above threshold counter
        ampSliceSum += amp;
        averageFreq += topOfSample;
        ampsAboveThresh += 1;
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
    //averageAmplitudeOfSlice[i] = ampSliceSum * weight;
    //if the current slice contains at least two amplitudes, then assign the average, otherwise assign the sum of the slice
    if (ampSliceCount > 1) {
      averageAmplitudeOfSlice[i] = int(round(ampSliceSum / ampSliceCount) * breadslicerSliceWeights[i]);
      averageFreq = int(round(averageFreq / ampsAboveThresh));
    } else {
      averageAmplitudeOfSlice[i] = round(ampSliceSum * breadslicerSliceWeights[i]);
    }
    // if there is at least one amplitude that is above threshold in the group, map it's frequency, otherwise frequency for that slice is unchanged
    if (ampsAboveThresh > 0) {
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
  amplitudeToRange = float(255.0 / (curMaxAvg * numWaves));
}


/**
 * Find all the major peaks in the fft spectrogram, based on the threshold and how many peaks to find 
 * 
 * 
 * @param float* data
 * @return N/A
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


/**
 * Finds the frequency with the most dominant change in amplitude, by comparing 2 consecutive FFT amplitude arrays 
 * 
 * 
 * @param float* data
 * @param float* prevData
 * @return N/A
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


/**
 * Records samples into the sample buffer array by reading from the audio input pins, at a sampling frequency of SAMPLE_FREQ
 * 
 * 
 * @param int sampleCount
 * @return N/A
 */
void recordSample(int sampleCount) {
  for (int i = 0; i < sampleCount; i++) {
    sampleT = mozziMicros();                                  // store the current time in microseconds
    int sampleL = mozziAnalogRead(AUDIO_INPUT_PIN_LEFT);    // Read sample from left input
    //int sampleR = mozziAnalogRead(AUDIO_INPUT_PIN_RIGHT);   // Read sample from right input
    vRealL[numSamplesTaken] = float(sampleL);       // store left channel sample in left vReal array
    vImagL[numSamplesTaken] = 0.0;                  // set imaginary values to 0.0
    //vRealR[numSamplesTaken] = float(sampleR);       // store right channel sampling in right vReal array
    //vImagR[numSamplesTaken] = 0.0;                  // set imaginary values to 0.0
    numSamplesTaken++;                              // increment sample counter
    // break out of loop if buffer is full, reset sample counter and increment the total number of sampling sessions
    if (numSamplesTaken == FFT_WIN_SIZE) {
      numSamplesTaken = 0;
      audioSamplingSessionsCount++;
      break;
    }
    // wait until sampleDelayTime has passed, to sample at the sampling frequency. Without this the DCRemoval() function will not work.
    while (1) {
      if (mozziMicros() - sampleT < sampleDelayTime) {
        break;
      }
    }
    continue;
  }
}

/*
########################################################
    Functions related to mapping amplitudes and frequencies generated by FFT analysis functions
########################################################
*/


/**
 * Used to precalculate the values for the fade function to reduce processor load during loop
 * 
 * 
 * @param N/A
 * @return N/A
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


/**
 * Maps the amplitudes and frequencies just generated by FFT to the next amplitudes to be synthesized, which is done to gradually increase/decrease
 * the real amplitudes to the next amplitudes with linear smoothing (can be changed to curve smoothing). 
 * 
 * @param N/A
 * @return N/A
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
        if (frequency < 260) {
          frequency = round(frequency * 0.4);
        // shrink the rest of the peak frequencies by mapping them to them to a range
        } else {
          // normalize the frequency to a value between 0.0 and 1.0
          float normalizedFreq = (frequency - 260.0) / (samplingFreqBy2 - 260.0);
          // this is done to do a logarithmic mapping rather than linear
          normalizedFreq = pow(normalizedFreq, 0.7);
          frequency = int(round(normalizedFreq * 140) + 40);
        }
        // assign new frequency to an ossilator, no need to smoothen this since the gain value for this ossilator is 0
        aSinL[i].setFreq(frequency);
      }
      // assign the gain value to the targetAmplitudeGains[i], the next waves to be synthesized
      nextAmplitudeGains[i] = gainValue;
    }
    // calculate the value to alter the amplitude per each update cycle
    ampGainStep[i] = float(nextAmplitudeGains[i] - prevAmplitudeGains[i]);
    // Serial.printf("(%d, %d, %03d, %03d)\t", i, i - aLocation, gainValue, frequency);
  }
  // Serial.println();
  // toggle between waves for which frequencies will be altered
  toggleFreqChangeWaves = 1 - toggleFreqChangeWaves;
}


/**
 * Linear smoothing between amplitudes to reduce audio glitches, using precalculated values
 * 
 * 
 * @param N/A
 * @return N/A
 */
void smoothenTransition() {
  for (int i = 0; i < totalNumWaves; i++) {
    //amplitudeGains[i] = round(prevAmplitudeGains[i] + FADE_CONST[fadeCounter] * ampGainStep[i]);
    // smoothen the transition between each amplitude, checking for difference to avoid additonal smoothing
    if (abs(nextAmplitudeGains[i] - int(round(amplitudeGains[i]))) > 2) {  
      amplitudeGains[i] = round(prevAmplitudeGains[i] + float(FADE_CONST[fadeCounter] * ampGainStep[i]));
    } else {
      amplitudeGains[i] = nextAmplitudeGains[i];
    }
  }
  // increment fade counter
  fadeCounter++;
}
