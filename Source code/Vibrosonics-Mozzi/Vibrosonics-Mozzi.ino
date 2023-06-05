/**@file Vibrosonics-Mozzi.ino */

/*/
########################################################

  READ ME FIRST!
    This code uses the arduinoFFT library and the Mozzi library. Before running this code, you have to ensure that all instances of "double" are replaced 
    with "float" in the arduinoFFT .cpp and .h sourcecode files. Using floating points instead of doubles is more suitable for the ESP32, and helps boost performance.
    
    The sourcecode is located in your libraries folder, usually under: "C:\Users\(YOUR NAME)\Documents\Arduino\libraries\arduinoFFT\src"

  DETAILS REGARDING THE CODE:

    This code uses the Mozzi library for audio synthesis, and it needs to have control over when certain processes are done, to consitently call
  updateAudio() @AUDIO_RATE times per second, which is done to fill the circular audio output buffer. For now, to fill the audio input buffer which is
  the size of @FFT_WIN_SIZE) this is using mozziAnalogRead() - similar to analogRead() but synced with Mozzi library. To do this without causing any
  timing issues and/or major audio glitches. All of audio input, fft analysis, signal processing, as well as modifications to the frequencies and amplitudes
  to synthesize, are completed in updateControl() - a function called by Mozzi @CONTROL_RATE times per second. We need to ensure that all of these functions
  execution time is within @UPDATE_TIME = (1000000 / @CONTROL_RATE) microseconds to ensure that updateAudio() and updateControl() calls are consistent.

  To ensure this, each updateControl() call is dedicated to a certain process, which are divided into four phases. 
    1. Phase A - The audio sampling phase - done however many times needed to fill the audio input buffer using recordSample() which analogRead() the number of
        samples that fit within @UPDATE_TIME at the @SAMPLING_FREQ. The number of samples to record per updateControl() call is calculated by
        @numSamplesPerProcess = @UPDATE_TIME / @sampleDelayTime. Moves onto the next phase when input buffer is full.
    2. Phase B - calling setupFFT() function to copy input buffer (@audioInputBuffer*) into the FFT input array (@vReal*) and calling FFT DCRemoval() to
        to reduce input noise
    3. Phase C - calls the most processor intensive function of FFT, which is the Windowing() function, this phase is dedicated to just this function
    4. Phase D - calls the remaining FFT functions FFTCompute() and ComplexToMagnitude(). This phase is also dedicated to all signal processing such
        signal processing of the output generated by the previous phases. The functions that are used for signal processing (averageFFTWindows(), breadslicer(),
        and freqMaxAmplitudeDelta()) are not very processor intensive, therefore they're all able to be called in the same process.
        NOTE: to use data from averaged FFT windows, pass "FFTData" as parameter to breadslicer, otherwise pass "vRealL"
    OTHER:
        - Along with all these phases and for every updateControl() call, the smoothTranstion() function is also called, this is a simple linear or curve based
        smoothing algorithm that uses pre-calculated values to smoothly transition amplitudes/frequencies between the previous and next amplitudes/frequencies this helps reduce audio
        glitches regarding sudden amplitude transitions, the number of smoothing steps is (@CONTROL_RATE / @SAMPLES_PER_SEC. 
        - For mapping the amplitudes and frequencies generated by signal processing, the mapFreqAmplitudes() is called right before the audioSamplingPhase. This ensures that smoothenTransition()
        works properly, by mapping the next amplitudes/frequencies for synthesis at the the same moment as when the previous amplitudes/frequencies are reached.

########################################################
/*/


#include <arduinoFFT.h>
#include <Arduino.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>

/*/
########################################################
    Directives
########################################################
/*/


// Audio input pins
#define AUDIO_INPUT_PIN_LEFT A2       // left channel audio input
#define AUDIO_INPUT_PIN_RIGHT A3      // right channel audio input

// FFT analysis rate and other FFT setup
#define SAMPLES_PER_SEC 32            // The number of FFT transforms done per second, this MUST be a power of 2. A higher value results in a higher frequency resolution but also increases delay

#define FFT_SAMPLING_FREQ 10000       // The sampling frequency, ideally should be a power of 2
#define FFT_WINDOW_SIZE int(pow(2, 12 - log(SAMPLES_PER_SEC) / log(2))) // the FFT window size and number of audio samples for ideal performance, this MUST be a power of 2
                                                                        // 2^(12 - log_base2(SAMPLES_PER_SEC)) = 64 for 64/s, 128 for 32/s, 256 for 16/s, 512 for 8/s
// Mozzi control rate
#define CONTROL_RATE int(pow(2, 5 + log(SAMPLES_PER_SEC) / log(2))) // Update control cycles per second for ideal performance, this MUST be a power of 2
                                                                    // 2^(5 + log_base2(SAMPLES_PER_SEC)) = 2048 for 64/s, 1024 for 32/s, 512 for 16/s, 256 for 8/s
// The number of waves to synthesize, and how many slices the breadslicer function does.
// Note: if BREADSLICER_USE_CURVE is 0, it's important to update the breadslicerSliceLocationsStatic array to match the number of waves
#define DEFAULT_NUM_WAVES 6

// These values are used for FFT signal processing functions such as breadslicer
#define BREADSLICER_USE_CURVE 0         // Set to 1 to use breadslicer curve for slicing, or 0 to set custom breadslicer values defined in @breadslicerSliceLocationsStatic*
const float BREADSLICER_CURVE_EXPONENT = 3.3;  // The exponent for the used for the breadslicer curve
const float BREADSLICER_CURVE_OFFSET = 0.55;   // The curve offset for the breadslicer to follow when slicing the amplitude array

// constants used for freqMaxAmplitudeDelta()
const int FREQ_MAX_AMP_DELTA_MIN = 200;   // The threshold for a change in amplitude to be considered significant by the frequencyMaxAmplitudeDelta() function, basically the sensitivity
const float FREQ_MAX_AMP_DELTA_K = 0.35;   // The K-value used to weight amplitudes that are not the amplitude of most change, the lower the value, the more extreme the affect

// mostly for noise, sensitivity and volume control
const int FFT_FLOOR_THRESH = 300;         // amplitude flooring threshold for FFT, to help reduce input noise
const int BREADSLICER_MAX_AVG_BIN = 3000; // The minimum max that is used for scaling the amplitudes to the 0-255 range, and helps represent the volume
const int MIN_WAVES_TO_SYNTH = 2;         // helps to represent volume of the incoming, the minimum number of waves to synthesize will be this value + 1

// These values define the min and max frequencies to use for synthesis
const int BASS_FREQ = 250;                // frequencies below this are considered to be bass
const float BASS_FREQ_SCALAR = 0.3;      // By how much to shrink frequencies low than BASS_FREQ

const int SYNTH_MIN_FREQ = 30;            // The mininum frequency to use for synthesizing frequencies above BASS_FREQ
const int SYNTH_MAX_FREQ = 140;           // The maximum frequency to use for synthesizing frequencies above BASS_FREQ
const float MAP_FREQ_EXPONENT = 0.8;      // The exponent to use when mapping the frequencies (1.0 is linear, 0.0 - 0.99 is "exponential", over 1.0 is "logarithmic")

/*
########################################################
    FFT
########################################################
*/


const uint16_t FFT_WIN_SIZE = int(FFT_WINDOW_SIZE);   // the windowing function size of FFT
const float SAMPLING_FREQ = float(FFT_SAMPLING_FREQ);   // the sampling frequency of FFT

// FFT_SIZE_BY_2 is FFT_WIN_SIZE / 2. We are sampling at double the frequency we are trying to detect, therefore only half of the vReal array is used for analysis post FFT
const int FFT_WINDOW_SIZE_BY2 = int(FFT_WIN_SIZE) >> 1;

const float SAMPLING_FREQ_BY2 = SAMPLING_FREQ / 2.0;  // the frequency we are sampling at divided by 2, since we need to sample twice the frequency we are trying to detect
const float frequencyResolution = float(SAMPLING_FREQ_BY2 / FFT_WINDOW_SIZE_BY2);  // the frequency resolution of FFT with the current window size

// Number of microseconds to wait between recording audio samples
const int sampleDelayTime = 1000000 / SAMPLING_FREQ;
// The total number of microseconds needed to record enough samples for FFT
const int totalSampleDelay = sampleDelayTime * FFT_WIN_SIZE;

// stores the samples read by the audio input pin
int audioInputBuffer[FFT_WIN_SIZE];
int numSamplesTaken = 0;    // stores the number of audio samples taken, used as iterator to fill audio sample buffer

float vRealL[FFT_WIN_SIZE];  // vRealL is used for input from the left channel and receives computed results from FFT
float vImagL[FFT_WIN_SIZE];  // vImagL is used to store imaginary values for computation

float vRealPrev[FFT_WINDOW_SIZE_BY2]; // stores the previous amplitudes generated by FFT

float FFTData[FFT_WINDOW_SIZE_BY2];   // stores the data the signal processing functions will use for the left channel

const float OUTLIER = 30000.0;  // Outlier for unusually high amplitudes in FFT

arduinoFFT FFT = arduinoFFT(vRealL, vImagL, FFT_WIN_SIZE, SAMPLING_FREQ); // Object for performing FFT's

/*
########################################################
    UPDATE RATE
########################################################
*/


const int UPDATE_TIME = 1000000 / CONTROL_RATE; // The estimated time in microseconds of each updateControl() call

int nextProcess = 0;  // The next signal aquisition/processing phase to be completed in updateControl
const int numSamplesPerProcess = floor(UPDATE_TIME / sampleDelayTime);  // The Number of samples to take per update
const int numProcessForSampling = ceil(FFT_WIN_SIZE / float(numSamplesPerProcess)); // the total number of processes to sample, calculated in setup
const int numTotalProcesses = numProcessForSampling + 3;  // adding 3 more processes for FFT windowing function and other processing

unsigned long sampleT;               // stores the time since program started using mozziMicros() rather than micros()

const int SAMPLE_TIME = int(CONTROL_RATE / SAMPLES_PER_SEC);  // sampling and processing time in updateControl
const int FADE_RATE = SAMPLE_TIME;                            // the number of cycles for fading
const float FADE_STEPX = float(2.0 / (FADE_RATE - 1));        // the x position of the smoothing curve

const float FADE_CONST_EXPONENT = 0.84;  // The exponent to use when generating a curve (1.0 is linear, for linear smoothing)
float FADE_CONST[FADE_RATE];  // Array storing amplitude smoothing constants from 0.0 to 1.0
int fadeCounter = 0;          // counter used to smoothen transition between amplitudes

/*
########################################################
    Stuff relevant to FFT signal processing functions
########################################################
*/


const int numWaves = int(DEFAULT_NUM_WAVES);

const int slices = numWaves;

float amplitudeToRange = (255.0 / BREADSLICER_MAX_AVG_BIN) / numWaves;   // the "K" value used to put the average amplitudes calculated by breadslicer into the 0-255 range. This value alters but BREADSLICER_MAX_AVG_BIN doesn't go lower than it's set value, and numWaves doesn't go above it's set value, to give a better sense of the volume

// if BREADSLICER_USE_CURVE == True then use breadslicerSliceLocations for slicing the FFT amplitudes array, otherwise use breadslicerSliceLocationsStatic
const int breadslicerSliceLocationsStatic[DEFAULT_NUM_WAVES] {250, 500, 900, 1600, 4400, 5000}; // array storing pre-defined slice locations in array for slicing the FFT amplitudes array (vReal)
int breadslicerSliceLocations[DEFAULT_NUM_WAVES]; // array for storing values caluclated for slicing the FFT amplitudes array

const float breadslicerSliceWeights[DEFAULT_NUM_WAVES] {1.0, 1.0, 1.0, 1.0, 1.0, 5.0}; // weights associated to the average amplitudes of slices

// These arrays are used to store the results calculated by breadslicer()
long averageAmplitudeOfSlice[DEFAULT_NUM_WAVES];  // the array used to store the average amplitudes calculated by the breadslicer
int peakFrequencyOfSlice[DEFAULT_NUM_WAVES];      // the array containing the peak frequency of the slice

// Used by frequencyMaxAmplitudeDelta() to compare the previous and current averageAmplitudeOfSlice*
long prevAverageAmplitudeOfSlice[DEFAULT_NUM_WAVES];  // the array used to store the previous average amplitudes calculated by the breadslicer

// Variables storing results calculated by frequencyMaxAmplitudeDelta()
float maxAmpChange = 0;     // the magnitude of the change
int maxAmpChangeIdx = 0;    // the array location of maxAmpChange
int maxAmpChangeDetected = 0;    // 0 or 1, based on whether or not a major amplitude change was detected

/*
########################################################
    Stuff relavent to additive synthesizer
########################################################
*/

Oscil<2048, AUDIO_RATE> aSinL[DEFAULT_NUM_WAVES];
//Oscil<2048, AUDIO_RATE> aSinR[DEFAULT_NUM_WAVES];

const int SYNTH_MAX_MIN_DIFF = int(SYNTH_MAX_FREQ) - int(SYNTH_MIN_FREQ); // used for mapping frequencies for synthesis

int amplitudeGains[DEFAULT_NUM_WAVES];        // the amplitudes used for audio synthesis
int prevAmplitudeGains[DEFAULT_NUM_WAVES];    // the previous amplitudes, used for smoothing transition
int nextAmplitudeGains[DEFAULT_NUM_WAVES];    // the next amplitudes
float ampGainStep[DEFAULT_NUM_WAVES];         // the difference between amplitudes transitions

int aSinLFrequencies[DEFAULT_NUM_WAVES];      // the frequencies used for audio synthesis
int prevWaveFrequencies[DEFAULT_NUM_WAVES];   // the previous frequencies
int nextWaveFrequencies[DEFAULT_NUM_WAVES];   // the next frequencies
float freqStep[DEFAULT_NUM_WAVES];            // the difference between frequency transitions

int updateCount = 0;       // updateControl() cycle counter
int audioSamplingSessionsCount = 0;  // counter for the sample and processing phases complete in a full CONTROL_RATE cycle, reset when updateCount is 0

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
  // wait for one second, not needed but helps prevent weird Serial output
  delay(1000);

  // setup audio input pins
  pinMode(AUDIO_INPUT_PIN_LEFT, INPUT);
  pinMode(AUDIO_INPUT_PIN_RIGHT, INPUT);

  // setup reference tables for aSin[] oscillator array and initialize arrays
  for (int i = 0; i < int(DEFAULT_NUM_WAVES); i++) {
    averageAmplitudeOfSlice[i] = 0;
    prevAverageAmplitudeOfSlice[i] = 0;
    aSinL[i].setTable(SIN2048_DATA);
    aSinLFrequencies[i] = int(map(breadslicerSliceLocations[i], 1, FFT_WINDOW_SIZE_BY2, 10, 160));
    amplitudeGains[i] = 0;
    prevAmplitudeGains[i] = 0;
    nextAmplitudeGains[i] = 0;
    prevWaveFrequencies[i] = aSinLFrequencies[i];
    nextWaveFrequencies[i] = aSinLFrequencies[i];
    ampGainStep[i] = 0.0;
    freqStep[i] = 0.0;
  }
  for (int i = 0; i < int(FFT_WINDOW_SIZE_BY2); i++) {
    vRealPrev[i] = 0.0;
    FFTData[i] = 0.0;
  }

  Serial.printf("FFT ANALYSIS PER SECOND: %d\tFFT WINDOW SIZE: %d\tFFT SAMPLING FREQUENCY: %d\tMOZZI CONTROL RATE: %d\tAUDIO RATE: %d\n", SAMPLES_PER_SEC, FFT_WIN_SIZE, FFT_SAMPLING_FREQ, CONTROL_RATE, AUDIO_RATE);
  Serial.println();
  // precalculate linear amplitude smoothing values and breadslicer slice locations to save processing power
  calculateFadeFunction();

  if (BREADSLICER_USE_CURVE) {
    calculateBreadslicerLocations();
  } else {
    saveBreadslicerLocations();
  }
  
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
 * audioHook() is the mozzi library wrapper for its loops and output. loop() is a Default Arduino function.
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
 * certain period of time that can be calculated by @UPDATE_TIME. Mozzi library function.
 * 
 * @param N/A
 * @return N/A
 */
void updateControl() {
  // reset update counter and audio sampling sessions counter
  if (updateCount >= CONTROL_RATE) {
    updateCount = 0;
    audioSamplingSessionsCount = 0;
  }

  // get the next available sample time in updateCount
  int sampleTime = SAMPLE_TIME * audioSamplingSessionsCount;
  // if it is time for next audio sample and analysis, begin the audio sampling phase, by setting next process 0
  if ((updateCount == 0 || updateCount == sampleTime - 1) && updateCount != CONTROL_RATE - 1) { 
    nextProcess = 0;
    fadeCounter = 0;
  }
  // Audio ampling phase
  if (nextProcess < numProcessForSampling) { 
    if (nextProcess == 0) {
      // map the frequencies and amplitudes generated by FFT signal processing functions right before the next audio sampling phase, to sync with smoothenTransition()
      mapFreqAmplitudes();
    }
    recordSample(); 
  }
  // FFT Phase A, functions
  else if (nextProcess == numTotalProcesses - 3) {
    // store previously computed FFT results in vRealPrev array, and copy values from audioInputBuffer to vReal array
    setupFFT();
    FFT.DCRemoval();  // Remove DC component of signal
  }
  // FFT phase B, only running the most processor intensive function during this process
  else if (nextProcess == numTotalProcesses - 2) {
    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
  }
  // FFT phase C, and signal processing phase
  else if (nextProcess == numTotalProcesses - 1) {
    FFT.Compute(FFT_FORWARD);   // Compute FFT
    FFT.ComplexToMagnitude();   // Compute frequency magnitudes

    // Average the previous and next vReal arrays for a smoother spectrogram
    averageFFTWindows();
    // Use the breadslicer function to process FFT output, pass "FFTData" as a parameter if using averaged FFT windows, otherwise pass "vRealL"
    breadslicer(vRealL);
    // Frequency of max amplitude change, to do additonal weighting of the values generated by breadslicer to help better represent transient events
    frequencyMaxAmplitudeDelta(averageAmplitudeOfSlice, prevAverageAmplitudeOfSlice, slices);
  }
  // called every updateControl tick, to smoothen transtion between amplitudes and change the sine wave frequencies that are used for synthesis
  smoothenTransition();

  // increment updateControl calls and nextProcess counters
  nextProcess++;
  // increment updateControl counter
  updateCount++;
}


/**
 * updateAudio() is called @AUDIO_RATE times per second, the code in here needs to be lightweight or audio glitches will occur.
 * Mozzi Library function.
 * 
 * @param N/A
 * @return N/A
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

/*
########################################################
    Functions related to FFT and FFT analysis
########################################################
*/


/**
 * Records samples into the sample buffer array by reading from the audio input pins, at a sampling frequency of SAMPLE_FREQ
 *
 * @param N/A
 * @return N/A
 */
void recordSample() {
  // record number of samples that will fit within @UPDATE_TIME time
  for (int i = 0; i < numSamplesPerProcess; i++) {
    sampleT = micros();
    audioInputBuffer[numSamplesTaken] = mozziAnalogRead(AUDIO_INPUT_PIN_LEFT);
    numSamplesTaken++;                              // increment sample counter
    // break out of loop if buffer is full, reset sample counter and increment the total number of sampling sessions
    if (numSamplesTaken == FFT_WIN_SIZE) {
      numSamplesTaken = 0;
      audioSamplingSessionsCount++;
      break;
    }
    // break out of loop if this was the last same for the current process (no need to wait)
    if (i == numSamplesPerProcess - 1) { break; }
    // wait until sampleDelayTime has passed, to sample at the sampling frequency. Without this the DCRemoval() function will not work.
    while (micros() - sampleT < sampleDelayTime) {
    };
  }
}


/**
 * setup arrays for FFT analysis
 * 
 * @param N/A
 * @return N/A
 */
void setupFFT() {
  for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
    // store the previously generated amplitudes in the vRealPrev array, to smoothen the spectrogram later during processing
    if (i < FFT_WINDOW_SIZE_BY2) {
      vRealPrev[i] = vRealL[i];
    }
    // copy values from audio input buffers
    vRealL[i] = float(audioInputBuffer[i]);
    // set imaginary values to 0
    vImagL[i] = 0.0;
  }
}


/**
 *  Average the previous and next FFT windows to reduce noise and produce a cleaner spectrogram for signal processing
 * 
 * @param N/A
 * @return N/A
 */
void averageFFTWindows () {
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i ++) {
    FFTData[i] = (vRealPrev[i] + vRealL[i]) / 2.0;
  }
}


/**
 *  Used to pre-calculate and save the array locations for the breadslicer using pre-defined frequency bins in breadslicerSliceLocationsStatic*, to reduce processor load during loop
 * 
 * @param N/A
 * @return N/A
 */
void saveBreadslicerLocations() {
  Serial.println("Calculating array slice locations for breadslicer...");
  Serial.print("\tUsing set frequency bins: ");
  for (int i = 0; i < slices; i++) {
    int slicePeakFrequency = breadslicerSliceLocationsStatic[i];
    Serial.printf("%dHz, ", slicePeakFrequency);
    int sliceIdx = 0;
    // find the closest matching frequency to the frequency stored in breadslicerSliceLocationsStatic[i]
    for (int j = 0; j < FFT_WINDOW_SIZE_BY2; j++) {
      sliceIdx++;
      if (int(round((j + 1) * frequencyResolution)) >= slicePeakFrequency) {
        breadslicerSliceLocations[i] = sliceIdx;
        break;
      }
      // if the frequency bin requested is not possible, set slice location to 0.
      if (j + 1 == FFT_WINDOW_SIZE_BY2) {
        breadslicerSliceLocations[i] = 0;
      }
    }
  }
  Serial.println();

  int lastSliceFrequency = 0;
  for (int i = 0; i < slices; i++) {
    // store location in array that is used by the breadslicer function
    int sliceLocation = breadslicerSliceLocations[i];
    int sliceFrequency = frequencyResolution * ((i == slices - 1) ? sliceLocation : sliceLocation - 1);
    Serial.printf("\t\tsliceLocation %d, Range %dHz to %dHz\n", sliceLocation, lastSliceFrequency, sliceFrequency);
    lastSliceFrequency = sliceFrequency;
  }
  Serial.println();
}


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
    //float exponent = 1.0 / (pow(xStep, BREADSLICER_CURVE_EXPONENT) + BREADSLICER_CURVE_OFFSET);  // exponent of the curve
    float exponent = 1.0 / 0.5;
    // calculate slice location in array based on the curve to follow for slicing.
    int sliceLocation = round(FFT_WINDOW_SIZE_BY2 * pow(xStep, exponent));
    int sliceFrequency = round(sliceLocation * frequencyResolution);      // Calculates the slices frequency range
    // The breadslicer function uses less-than sliceLocation in its inner loop, which is why we add one.
    if (sliceLocation < FFT_WINDOW_SIZE_BY2) {
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
 * Splits the amplitude data (vReal array) into X bands, defined by breadslicerSliceLocations* This is done to detect features of sound which are commonly found at certain frequency ranges.
 * For example, 0-200Hz is where bass is common, 200-1200Hz is where voice and instruments are common, 1000-5000Hz higher notes and tones as well as harmonics 
 * 
 * @param float* data | The vReal[] array containing post-FFT audio signal data.
 * @param int sliceInto | Number of slices to be made.
 * @return N/A
 */
void breadslicer(float *data) {
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

    // if there is at least one amplitude that is above threshold in the group, map it's peak frequency, otherwise frequency for that slice is unchanged
    if (ampSliceCount > 0) {
      peakFrequencyOfSlice[i] = maxSliceAmpFreq;
    }
    // similar to getting the integral of the slice, but upon further testing this didn't seem to affect the performance much, but i'll leave it here for future teams to experiment
    //ampSliceSum *= frequencyResolution;
    //if the current slice contains at least two amplitudes, then assign the average, otherwise assign the sum of the slice
    if (ampSliceCount > 1) {
      averageAmplitudeOfSlice[i] = int(round(ampSliceSum / ampSliceCount));
    } else {
      averageAmplitudeOfSlice[i] = round(ampSliceSum);
    }
    averageAmplitudeOfSlice[i] = round(averageAmplitudeOfSlice[i] * breadslicerSliceWeights[i]);
    // set the iterator to the next location in the array for the next slice
    lastJ = newJ;
  }
}


/**
 * Finds the frequency with the most dominant change in amplitude, by comparing 2 consecutive FFT amplitude arrays 
 * 
 * @param float* data | The averageAmplitudeOfSlice[] array containing average amplitude values in all breadslicer slices.
 * @param float* prevData | The preAverageAmplitudeOfSlice[] array, or the prior iteration of averageAmplitudeofSlice[].
 * @param int arraySize | The length of the data[] and prevData[] arrays. Equal to the current number of slices.
 * @return N/A
 */
void frequencyMaxAmplitudeDelta(long *data, long *prevData, int arraySize) {
  // restore global varialbes
  maxAmpChangeDetected = 0;
  maxAmpChange = 0.0;
  // iterate through data* and prevData* to find the amplitude with most change
  for (int i = 0; i < arraySize; i++) {
    if (data[i] < OUTLIER) {
      // store the change of between this amplitude and previous amplitude
      int currAmpChange = abs(data[i] - prevData[i]);
      // find the most dominant amplitude change and store in maxAmpChangeIdx, store the magnitude of the change in maxAmpChange
      if (currAmpChange > maxAmpChange && currAmpChange > FREQ_MAX_AMP_DELTA_MIN) {
        maxAmpChangeDetected = 1;
        maxAmpChange = currAmpChange;
        maxAmpChangeIdx = i;
      }
    }
    // assign data to previous data
    prevData[i] = data[i];
  }
}

/*
########################################################
  Functions related to mapping amplitudes and frequencies generated by FFT analysis functions for synthesis
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
      FADE_CONST[i] = -(pow(abs(xStep), FADE_CONST_EXPONENT) * 0.5) + 0.5;
    } else {
      FADE_CONST[i] = pow(xStep, FADE_CONST_EXPONENT) * 0.5 + 0.5;
    }
    // Serial.printf("\tfadeCounter: %d, xValue: %0.2f, yValue: %.2f\n", i, xStep, FADE_CONST[i]);
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
  // store the current max amplitude to put amplitudes in 0-255 range
  int maxAmp = BREADSLICER_MAX_AVG_BIN;
  // count amplitudes above threshold
  int numWavesAboveThresh = MIN_WAVES_TO_SYNTH;
  // ensure that averages aren't higher than the max set average value, and if it is higher, then the max average is replaced by that value, for this iteration
  for (int i = 0; i < slices; i++) {
    if (i != maxAmpChangeIdx && maxAmpChangeDetected) { 
      // weight the slice with most dominant change, based on the frequencyMaxAmplitudeDelta()
      averageAmplitudeOfSlice[i] = long(round(averageAmplitudeOfSlice[i] * FREQ_MAX_AMP_DELTA_K));
    }
    // increment numWavesAboveThresh if amplitude is above 0 and as long as the numWavesAboveThresh is less than @numWaves
    if (averageAmplitudeOfSlice[i] > 0 && numWavesAboveThresh < numWaves) {
      numWavesAboveThresh++;
    }
    if (averageAmplitudeOfSlice[i] > maxAmp) {
    maxAmp = averageAmplitudeOfSlice[i];
    }
  }
  // put all amplitudes within the 0-255 range, with this "K" value
  amplitudeToRange = float((255.0 / maxAmp) / ((numWavesAboveThresh > 0) ? numWavesAboveThresh : 1.0));

  // for the total number of waves, assign the frequency and the amplitudes just generated by FFT to the amplitudes and frequencies of sine waves that will be synthesized
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
      // multiply the gain value by a "K" value to put it into the 0-255 range, this value alters based on the max average amplitude but doesn't go below a certain point
      gainValue = round(gainValue * amplitudeToRange);
      frequency = peakFrequencyOfSlice[i];
      // slightly shrink down the bassy frequencys
      if (frequency < BASS_FREQ) {
        frequency = int(round(frequency * float(BASS_FREQ_SCALAR)));
      // shrink the rest of the peak frequencies by mapping them to them to a range
      } else {
        // normalize the frequency to a value between 0.0 and 1.0
        float normalizedFreq = (float(frequency - BASS_FREQ) / float(SAMPLING_FREQ_BY2 - BASS_FREQ));
        // this is done to do a logarithmic mapping rather than linear
        normalizedFreq = pow(normalizedFreq, MAP_FREQ_EXPONENT);
        frequency = int(round(float(normalizedFreq * SYNTH_MAX_MIN_DIFF) + SYNTH_MIN_FREQ));
      }
      nextWaveFrequencies[i] = frequency;
    }
    nextAmplitudeGains[i] = gainValue;
    // assign the gain value to the nextAmplitudeGains[i], the next waves to be synthesized
    // calculate the value to alter the amplitude per each update cycle
    ampGainStep[i] = float(nextAmplitudeGains[i] - prevAmplitudeGains[i]);
    freqStep[i] = float(nextWaveFrequencies[i] - prevWaveFrequencies[i]);
    // Serial.printf("(%d, %03d, %03d)\t", i, gainValue, frequency);
  }
  // Serial.println();
}


/**
 * Linear smoothing between amplitudes to reduce audio glitches, using precalculated values
 * 
 * 
 * @param N/A
 * @return N/A
 */
void smoothenTransition() {
  for (int i = 0; i < numWaves; i++) {
    // smoothen the transition between each amplitude, checking for difference to avoid additonal smoothing
    if (abs(nextAmplitudeGains[i] - amplitudeGains[i]) > 2) {  
      amplitudeGains[i] = int(round(float(prevAmplitudeGains[i] + FADE_CONST[fadeCounter] * ampGainStep[i])));
    } else {
      amplitudeGains[i] = nextAmplitudeGains[i];
    }
    // smoothen the transition between each frequencies
    aSinLFrequencies[i] = int(round(float(prevWaveFrequencies[i] + FADE_CONST[fadeCounter] * freqStep[i])));
    aSinL[i].setFreq(aSinLFrequencies[i]);
  }
  fadeCounter++;
}