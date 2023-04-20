/*/
########################################################
  READ ME FIRST! This code uses the arduinoFFT library and the Mozzi library. Before running this code, you have to replace all instances of
  "double" with "float" in the arduinoFFT .cpp and .h sourcecode files. Using floating points instead of doubles is more suitable for the ESP32,
  and helps boost performance. The sourcecode is located in your libraries folder, usually under:
    C:\Users\(YOUR NAME)\Documents\Arduino\libraries\arduinoFFT\src
########################################################
  
########################################################
/*/

#include <arduinoFFT.h>
#include <Arduino.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>
#include <mozzi_midi.h>
#include <mozzi_fixmath.h>
#include <EventDelay.h>
#include <string.h>

/*/
########################################################
    Directives
########################################################
/*/

#define AUDIO_INPUT_PIN_LEFT A2   // left channel audio input
#define AUDIO_INPUT_PIN_RIGHT A3  // right channel audio input

#define SAMPLES_PER_SEC 32  // The number of sampling/analysis cycles per second, this MUST be a power of 2

#define FFT_SAMPLING_FREQ 10000                                                      // The sampling frequency, ideally should be a power of 2
//#define FFT_SAMPLING_FREQ_BASS 4096
#define FFT_WINDOW_SIZE int(pow(2, int(12 - (log(int(SAMPLES_PER_SEC)) / log(2)))))  // the FFT window size and number of audio samples for ideal performance, this MUST be a power of 2 \
                                                                                     // 2^(12 - log_base2(SAMPLES_PER_SEC)) = 64 for 64/s, 128 for 32/s, 256 for 16/s, 512 for 8/s

#define FFT_FLOOR_THRESH 200  // amplitude flooring threshold for FFT, to reduce noise

#define CONTROL_RATE int(pow(2, int(5 + (log(int(SAMPLES_PER_SEC)) / log(2)))))  // Update control cycles per second for ideal performance, this MUST be a power of 2
                                                                                 // 2^(5 + log_base2(SAMPLES_PER_SEC)) = 2048 for 64/s, 1024 for 32/s, 512 for 16/s, 256 for 8/s

#define OSCIL_COUNT 16  // The total number of waves to synthesize

#define DEFAULT_NUM_WAVES 5            // The number of waves to synthesize, and how many slices the breadslicer function does

#define DEFAULT_BREADSLICER_CURVE 0.5  // The default curve for the breadslicer to follow when slicing the amplitude array
#define DEFAULT_BREADSLICER_MAX_AVG_BIN 1500 // The minimum max that is used for scaling the amplitudes to the 0-255 range, and helps represent the volume

/*/
########################################################
    AUDIO SAMPLING
########################################################
/*/

// volatile int test = 0;

// hw_timer_t *sampling_timer = NULL;
// void IRAM_ATTR onTimer() {
//   test += 1;
// }

int silenceTime = 0;                               // the total time the microphone input was minimal each increment of silenceTime is equal to (1 second / CONTROLRATE)
const int maxSilenceTime = 1 * int(CONTROL_RATE);  // the maximum time that the microphone can be mininal, used to silence the audio output after a certain time (5 seconds in this case)

unsigned long microseconds;

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

// Number of microseconds to wait between recording samples.
const int sampleDelayTime = 1000000 / SAMPLE_FREQ;
const int totalSampleDelay = sampleDelayTime * FFT_WIN_SIZE;

int numSamplesTaken = 0;

float vRealL[FFT_WIN_SIZE];  // vRealL is used for input from the left channel and receives computed results from FFT
float vImagL[FFT_WIN_SIZE];  // vImagL is used to store imaginary values for computation
float vRealR[FFT_WIN_SIZE];  // vRealR is used for input from the right channel and receives computed results from FFT
float vImagR[FFT_WIN_SIZE];  // vImagR is used to store imaginary values for computation


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

int nextProcess = 0;                                                               // The next signal aquisition/processing phase to be completed in updateControl
const int numSamplesPerProcess = floor(UPDATE_TIME / sampleDelayTime);            // The Number of samples to take per update
const int numProcessForSampling = (FFT_WIN_SIZE / numSamplesPerProcess) + 1;      // The number of update calls needed to sample audio
const int numTotalProcesses = numProcessForSampling + 2;                          // adding 2 more processes for FFT windowing function and other processing

int nextSamplingDelay = sampleDelayTime - (UPDATE_TIME - numSamplesPerProcess * sampleDelayTime);

const int SAMPLE_TIME = int(CONTROL_RATE / SAMPLES_PER_SEC);  // sampling and processing time in updateControl
const int FADE_RATE = SAMPLE_TIME;                            // the number of cycles for fading
const float FADE_STEPX = float(2.0 / (FADE_RATE - 1));              // the x position of the smoothing curve

float FADE_CONST[FADE_RATE];

int fadeCounter = 0;

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

float amplitudeToRange = (255.0/DEFAULT_BREADSLICER_MAX_AVG_BIN) / numWaves;   // the "K" value used to put the average amplitudes calculated by breadslicer into the 0-255 range. This value alters but doesn't go below DEFAULT_BREADSLICER_MAX_AVG_BIN to give a sense of the volume
float bySlices = 1.0 / numWaves;                  // The linear x-step that is used to traverse through the curve used by the breadslicer
float curve = float(DEFAULT_BREADSLICER_CURVE);   // curve accounts for the curve used when splitting the slices

int breadslicerSliceLocations[DEFAULT_NUM_WAVES];
float breadslicerSliceWeights[DEFAULT_NUM_WAVES] = {0.5, 1.0, 1.0, 1.0, 0.8};

int averageAmplitudeOfSlice[OSCIL_COUNT];          // the array used to store the average amplitudes calculated by the breadslicer
int peakFrequencyOfSlice[OSCIL_COUNT];              // the array containing the peak frequency of the slice

/*/
########################################################
    Stuff relavent to additive synthesizer
########################################################
/*/

EventDelay kChangeBinsDelay;
const unsigned int gainChangeMsec = 1000;

Oscil<2048, AUDIO_RATE> aSinL[OSCIL_COUNT];
//Oscil<2048, AUDIO_RATE> aSinR[OSCIL_COUNT];

int amplitudeGains[OSCIL_COUNT];      // the amplitudes used for synthesis
int prevAmplitudeGains[OSCIL_COUNT];    // the previous amplitudes, used for curve smoothing
int nextAmplitudeGains[OSCIL_COUNT];    // the target amplitudes
int targetAmplitudeGains[OSCIL_COUNT];  // the target amplitudes
float ampGainStep[OSCIL_COUNT];         // the difference between amplitudes transitions

int targetWaveFrequencies[OSCIL_COUNT];  // array containing target frequencies used for synthesis

int toggleFreqChangeWaves = 0;  // toggle frequency transtions between next available waves and the previous waves, to change the frequencies of the next available waves (ones that are at 0 amplitude)

int8_t carrierLeft = 0;
int8_t carrierRight = 0;

int updateCount = 0;       // control rate cycle counter
int audioSamplingSessionsCount = 0;  // sample and processing phases complete in control_rate cycle

/*/
########################################################
  Other
########################################################
/*/

int opmode = 0;

char outputString[500];

/*/
########################################################
    Setup
########################################################
/*/

void setup() {
  // set baud rate
  Serial.begin(115200);

  // samplingTimer = timerBegin(2, 8000, true);
  // timerAttachInterrupt(samplingTimer, &onTimer, true);
  // timerAlarmWrite(samplingTimer, 100000, true);
  // timerAlarmEnable(samplingTimer);

  // setup reference tables for aSin[] oscillator array
  for (int i = 0; i < OSCIL_COUNT; i++) {
    aSinL[i].setTable(SIN2048_DATA);
    //aSinR[i].setTable(SIN2048_DATA);
    amplitudeGains[i] = 0;
    targetAmplitudeGains[i] = 0;
    ampGainStep[i] = 0.0;
    targetWaveFrequencies[i] = 0;
    averageAmplitudeOfSlice[i] = 0;
  }

  // setup audio input pin
  pinMode(AUDIO_INPUT_PIN_LEFT, INPUT);
  pinMode(AUDIO_INPUT_PIN_RIGHT, INPUT);

  for (int i = 0; i < FFT_WIN_SIZE_BY2; i++) {
    vRealPrev[i] = 0.0;
    peakAmplitudes[i] = -1;
  }

  calculateFadeFunction();
  calculateBreadslicerLocations();
  
  // Additive Synthesis
  startMozzi(CONTROL_RATE);

  while (!Serial)
    ;
  Serial.println("Ready");
}

/*/
########################################################
    Loop
########################################################
/*/

void loop() {
  audioHook();
}

/*/
########################################################
    Additive Synthesis Functions
########################################################
/*/

void updateControl() {
  /* reset update counter */
  if (updateCount >= CONTROL_RATE) {
    updateCount = 0;
    audioSamplingSessionsCount = 0;
  }
  //Serial.print(updateCount);

  /* Output string */
  //memset(outputString, 0, sizeof(outputString));

  int sampleTime = SAMPLE_TIME * audioSamplingSessionsCount;  // get the next available sample time
  // if it is time for next audio sample and analysis, begin the audio sampling phase, by setting next process to 0
  if ((updateCount == 0 || updateCount == sampleTime - 1) && updateCount != CONTROL_RATE - 1) {
    nextProcess = 0;
    fadeCounter = 0;
    //nextSamplingDelay = 0;
  }

  //unsigned long execT = micros();

  /* audio sampling phase */
  if (nextProcess < numProcessForSampling) {
    //delay(nextSamplingDelay);
    recordSample(numSamplesPerProcess);
    // Serial.print("audio sampling: ");
    // Serial.println(micros() - execT);
  }

  /* FFT phase*/
  else if (nextProcess == numTotalProcesses - 2) {
    FFT.DCRemoval();  // Remove DC component of signal
    //Serial.print("dc remvoal: ");
    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
    // Serial.print("windowing: ");
    // Serial.println(micros() - execT);
  }
  /* next FFT and processing phase*/
  else if (nextProcess == numTotalProcesses - 1) {
    // sampleAvgPrint = round(sampleAvg / FFT_WIN_SIZE);
    // sampleAvg = 0;
    // currSampleMin = 5000;
    // currSampleMax = 0;

    /* Compute FFT */
    FFT.Compute(FFT_FORWARD);

    //Serial.print("FFT Cycle: ");

    /* Compute frequency magnitudes */
    FFT.ComplexToMagnitude();
    //===
    //hpsAnalysis();
    //crudeFindFormant();

    // Serial.print("FFT Cycle: ");
    // Serial.println(micros() - execT);

    /* Breadslicer */
    breadslicer(vRealL, slices);

    /* Frequency of max amplitude change */
    //frequencyMaxAmplitudeDelta();

    /* Major peaks of an FFT spectrogram */
    //findMajorPeaks();
    // for (int i = 0; i <= numWaves; i++) {
    //   Serial.printf("(%d, %d),", peakIndexes[i], peakAmplitudes[i]);
    // }
    // Serial.println();

    /* mapping each average normalized bin from FFT analysis to a sine wave amplitude for additive synthesis */
    mapFreqAmplitudes();
    // for (int i = 0; i < numWaves; i++) {
    //   Serial.printf("(%03d, %03d), ", targetWaveFrequencies[i], targetAmplitudeGains[i]);
    // }
    // Serial.println();
  }
  crossfadeAmpsFreqs();

  /* Serial Ouput */
  // char buffer3[128];
  //Serial.printf("Plotter: Min:%d\tMax:%d\tOutput(0-255):%03d\tInput(0-255):%03d\t\n", 0, 255, map(carrier, -127, 128, 0, 255), map(sampleAvgPrint, 0, sampleMax, 0, 255));
  // strcat(outputString, buffer3);

  // increment updateControl calls counter
  nextProcess++;
  updateCount++;
  //Serial.print(outputString);
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
  // just generated by FFT to amplitudes and sine waves that will be synthesized
  for (int i = 0; i < totalNumWaves; i++) {
    // kind of like a circular buffer to smoothen transition between amplitudes to reduce audio glitches,
    // results in a delay between input and output of about ((1/SAMPLES_PER_SEC) * 2) seconds
    prevAmplitudeGains[i] = nextAmplitudeGains[i];
    nextAmplitudeGains[i] = targetAmplitudeGains[i];

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
        if (frequency < 250) {
          frequency = round(frequency * 0.4);
        // shrink the rest of the peak frequencies by mapping them to them to a range
        } else {
          // normalize the frequency to a value between 0.0 and 1.0
          float normalizedFreq = (frequency - 250.0) / (samplingFreqBy2 - 250.0);
          // this is done to do a logarithmic mapping rather than linear
          normalizedFreq = pow(normalizedFreq, 0.7);
          frequency = int(round(normalizedFreq * 140) + 40);
        }
        // assign new frequency to an ossilator, no need to smoothen this since the gain value for this ossilator is 0
        aSinL[i].setFreq(frequency);
      }
      //Serial.printf("(%d, %d, %03d, %03d)\t", i, i - aLocation, gainValue, frequency);
      // assign the gain value to the targetAmplitudeGains[i], the next waves to be synthesized
      targetAmplitudeGains[i] = gainValue;
    }
    // calculate the value to alter the amplitude per each update cycle
    ampGainStep[i] = float(nextAmplitudeGains[i] - prevAmplitudeGains[i]);
  }
  //Serial.println();
  // toggle between waves for which frequencies will be altered
  toggleFreqChangeWaves = 1 - toggleFreqChangeWaves;
}

void crossfadeAmpsFreqs() {
  for (int i = 0; i < totalNumWaves; i++) {
    //amplitudeGains[i] = round(prevAmplitudeGains[i] + FADE_CONST[fadeCounter] * ampGainStep[i]);
    // smoothen the transition between each amplitude, checking for difference to avoid additonal smoothing
    if (abs(nextAmplitudeGains[i] - int(round(amplitudeGains[i]))) > 1) {  
      amplitudeGains[i] = round(prevAmplitudeGains[i] + FADE_CONST[fadeCounter] * ampGainStep[i]);
    } else {
      amplitudeGains[i] = nextAmplitudeGains[i];
    }
    //Serial.printf("(%.2f, %03d, %03d), ", amplitudeGains[i], nextAmplitudeGains[i], targetAmplitudeGains[i]);
  }
  // increment fade counter
  fadeCounter++;
  //Serial.println();
}

AudioOutput_t updateAudio() {
  int longCarrierLeft = 0;
  //int longCarrierRight = 0;

  for (int i = 0; i < totalNumWaves; i++) {
    longCarrierLeft += amplitudeGains[i] * aSinL[i].next();
    //longCarrierRight += amplitudeGains[i] * aSinR[i].next();
    // if (amplitudeGains[i] != 0) {
    //   longCarrierLeft += amplitudeGains[i] * aSinL[i].next();
    // }
  }
  carrierLeft = int8_t(longCarrierLeft >> 8);
  //carrierRight = int8_t(longCarrierRight >> 8);

  return carrierLeft;
  //return StereoOutput::from8Bit(carrierLeft, carrierLeft);
}

/*/
########################################################
    FTT processing
########################################################
/*/

/*
  Used to pre-calculate the array locations for the breadslicer to reduce processor load during loop
*/
void calculateBreadslicerLocations() {
  // The curve to follow for binning the amplitudes array: (x/slices)^(1/(x + curveValue))
  float step = 1.0 / slices;       // how often to step on the x-axis to determine bin value
  // calculate array locations for the breadslicer
  Serial.println("Calculating array slice locations for breadslicer...");
  // stores the previous slices frequency, only used for printing
  int lastSliceFrequency = 0;
  for (int i = 0; i < slices; i++) {
    float xStep = (i + 1) * step;        // x-axis step (0.0 - 1.0)
    float exponent = 1.0 / (pow(xStep, 10.0) + DEFAULT_BREADSLICER_CURVE);  // exponent of the curve (1 / (x^10 - DEFAULT_BREADSLICER_CURVE))
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
    Serial.printf("\sliceLocation %d, Range %dHz to %dHz\n", sliceLocation, lastSliceFrequency, sliceFrequency);
    // store the previous slices frequency
    lastSliceFrequency = sliceFrequency;
  }
  Serial.println("\n");
}

/*
  Used to precalculate the values for the fade function to reduce processor load during loop
*/
void calculateFadeFunction() {
  Serial.println("Pre-calculating fade function constants...");
  for (int i = 0; i < FADE_RATE; i++) {
    float xStep = i * FADE_STEPX - 1.0;
    if (xStep < 0.0) {
      FADE_CONST[i] = -(pow(abs(xStep), 1.0) * 0.5 + 0.5);
    } else {
      FADE_CONST[i] = pow(xStep, 1.0) * 0.5 + 0.5;
    }
    Serial.printf("\tfadeCounter: %d, Value: %.2f\n", i, FADE_CONST[i]);
  }
  Serial.println();
}

/*
  Splits the amplitude data (vReal array) into X bands based on somewhat of a parabolic/logarithmic curve, where X is sliceInto. This is done to detect features of sound which are commonly found at certain frequency ranges.
  For example, 0-200Hz is where bass is common, 200-1200Hz is where voice and instruments are common, 1000-5000Hz higher notes and tones as well as harmonics 
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

    // amplitude weighting based on how many amplitudes are in each slice. (FFT_WIN_SIZE_BY2 - 1), since we are excluding the first location in the array
    float weight = float((FFT_WIN_SIZE_BY2 - 1) - (sliceSize - lastJ)) / (FFT_WIN_SIZE_BY2 - 1);
    //float weight = float(sliceSize - lastJ) / (FFT_WIN_SIZE_BY2 - 1);
    //Serial.println(weight);

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
      // amplitude assigned and weighting based on how many amplitudes are in the current slice
      int amp = round(data[j]);
      // if amplitude is above certain threshold, exclude from signal processing
      if (amp > FFT_FLOOR_THRESH && amp < OUTLIER) {
        // floor the current amplitude, considered 0 otherwise since its excluded
        //amp -= FFT_FLOOR_THRESH;//amp * weight - FFT_FLOOR_THRESH;
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

      // Debugging stuff
      // Serial.printf("I: %d\tFreq: %.2f\tAmp: %.2f\t", j, topOfSample, data[j]);
      // Serial.println();

      // save last location in array
      newJ += 1;
    }
    //averageAmplitudeOfSlice[i] = ampSliceSum * weight;
    //if the current slice contains at least two amplitudes, then assign the average, otherwise assign the sum of the slice
    if (ampSliceCount > 1) {
      averageAmplitudeOfSlice[i] = int(round(ampSliceSum / ampSliceCount) * breadslicerSliceWeights[i]);
      averageFreq = int(round(averageFreq / ampsAboveThresh));
    } else {
      averageAmplitudeOfSlice[i] = round(ampSliceSum * breadslicerSliceWeights[i]);
    }

    // if there is at least one amplitude in the group, map it's frequency, otherwise frequency for that slice is unchanged
    if (ampsAboveThresh > 0) {
      peakFrequencyOfSlice[i] = maxSliceAmpFreq;
    }
  
    // ensure that averages aren't higher than the max set average value, and if it is higher, then the max is replaced by that value, for this iteration
    if (averageAmplitudeOfSlice[i] > curMaxAvg) {
        curMaxAvg = averageAmplitudeOfSlice[i];
    }
    // set the iterator to the next location in the array for the next slice
    lastJ = newJ;
  }
  // put all amplitudes within the 0-255 range, with this "K" value, done in mapFreqAmplitudes()
  amplitudeToRange = float(255.0 / (curMaxAvg * numWaves));
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
  targetWaveFrequencies[numWaves] = map(freqMaxAmpChange, 0, int(samplingFreqBy2), 16, 200);
}

/* 
  Records samples into the sample buffer array by reading from the audio input pins, at a sampling frequency of SAMPLE_FREQ
*/
void recordSample(int sampleCount) {
  for (int i = 0; i < sampleCount; i++) {
    microseconds = micros();                                  // store the current time in microseconds
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
    while (micros() - microseconds < sampleDelayTime) {}
  }
}

/*/
########################################################
    Formant Estimation 
########################################################
/*/
//Downsample x times, and then compare peaks in each window, if there are matching peaks, throw a positive for a formant

static int downsampleCount = 5;  //How many times to downsample. More generally gives higher precision, but requires more processing power. Refers to number of harmonics being considered.
//int downsampleFactor = 2; //Factor to downsample by. Setting to 2 means we're picking every other data point when downsampling.
//static int lpcOrder = 10;

int maleFormantTable[6];


float thisWindow[FFT_WIN_SIZE / 2];

void crudeFindFormants()
{
  maleFormantTable[0] = 2160; // i
  maleFormantTable[2] = 1865; // y
  maleFormantTable[3] = 1920; // e
  maleFormantTable[3] = 760; // a
  maleFormantTable[4] = 280; // o
  maleFormantTable[5] = 340; // u

  float maxAmpOne = 0;
  float maxAmpTwo = 0;
  int strongFreqOne = 0;
  int strongFreqTwo = 0;
  //Potentially add a 3rd to account for fundemental freq.

  for(int i = 2; i < FFT_WIN_SIZE / 2; i++)
  {
    //Serial.println(vReal[i]);
  
    if(vReal[i] > 100)
    {
      //Serial.printf("---%f---%f\n", vReal[i], maxAmpOne);
      if(vReal[i] > maxAmpOne)
      {
        maxAmpOne = vReal[i];
        strongFreqOne = i;
        continue;
      }
      else if(vReal[i] > maxAmpTwo)
      {
        maxAmpTwo = vReal[i];
        strongFreqTwo = i;
      }
    }
      //Serial.println(i);
  }

  for(int i = 0; i < 6; i++)
  {
      if(abs(strongFreqOne - strongFreqTwo) > maleFormantTable[i] - maleFormantTable[i] * 0.08 && abs(strongFreqOne - strongFreqTwo) < maleFormantTable[i] + 30)
      {
        Serial.printf("Formant Found! 1-%d | 2-%d | Difference - %d\n", strongFreqOne, strongFreqTwo, abs(strongFreqOne - strongFreqTwo));
      }
      else
      {
        Serial.printf("Formant not Found 1-%d | 2-%d | Difference - %d\n", strongFreqOne, strongFreqTwo, abs(strongFreqOne - strongFreqTwo));
      }
  }


}


const int FORMANT_ARRAY_SIZE = 8;
int formantIndex[FORMANT_ARRAY_SIZE]; //Size depends on how many potential formants we want to identify

void hpsAnalysis()
{
  int fundFreqIndex = 0;
  //Serial.println("inHPS!");
  for(int i = 0; i < FFT_WIN_SIZE / 2; i++)
  {
    thisWindow[i] = 1;
    //Serial.println(thisWindow[i]);
    
  }
  for(int i = 1; i < downsampleCount; i++)
  {
    int k = 0;
    while(k < FFT_WIN_SIZE / 2)
    {
      //Serial.println(k);
      if(vReal[k] > 0 && vReal[k] < OUTLIER)
      {
        thisWindow[k] = thisWindow[k] * vReal[k];
      }
      if(k + i > FFT_WIN_SIZE / 2)
      {
        break;
      }
      k++;
      k += i;
    }
  }

  for(int i = 0; i < FFT_WIN_SIZE / 2; i++)
  {
    //Serial.printf("%f--->%f\n", thisWindow[i], thisWindow[fundFreqIndex]);
    if(thisWindow[i] > thisWindow[fundFreqIndex])
    {
      shiftArray(formantIndex, 0, fundFreqIndex, FORMANT_ARRAY_SIZE);
      fundFreqIndex = i;
    }
    else
    {
      for (int k = 0; k < FORMANT_ARRAY_SIZE; k++) //If the amplitude at "i" frequency is greater than the amplitude at index "k" frequency, replace.
      {
        if(thisWindow[i] > thisWindow[formantIndex[k]]) //Creates an array of highest-amplitude non-fundamental frequencies (ideally), going from largest to smallest 
        {
          shiftArray(formantIndex, k, i, FORMANT_ARRAY_SIZE);
          break;
        }
      }
    }
  }
  //Serial.println("beforeCompare");
  compareFormants();

}

void compareFormants()
{
  /*for(int i = 0; i < FORMANT_ARRAY_SIZE; i++)
  {
    Serial.println(formantIndex[i]);
  }*/

  for(int j = 1; j < FORMANT_ARRAY_SIZE; j++)
  {
    for(int k = j + 1; k < FORMANT_ARRAY_SIZE; k++)
    {
      for(int i = 0; i < 6; i++)
      {
          if(abs(formantIndex[j] - formantIndex[k]) > maleFormantTable[i] - maleFormantTable[i] * 0.08 && 
          abs(formantIndex[j] - formantIndex[k]) < maleFormantTable[i] + maleFormantTable[i] * 1.08)
          {
          Serial.printf("Formant Found! 1-%d | 2-%d | Difference - %d\n", formantIndex[j], formantIndex[k], 
          abs(formantIndex[j] - formantIndex[k]));
          }
          else
          {
           Serial.printf("Formant not Found 1-%d | 2-%d | Difference - %d\n", formantIndex[j], formantIndex[k], 
           abs(formantIndex[j] - formantIndex[k]));
          }
      }
    }
  }
  //Serial.println("hpsDone!");
}

void shiftArray(int *array, int replaceIndex, int newValue, int arraySize)
{
  int temp;
  //Serial.printf("%d--->%d at %d\n", array[replaceIndex], newValue, replaceIndex);

  if(replaceIndex == arraySize - 1)
  {
    array[replaceIndex] = newValue;
    return;
  }
  else
  {
    for(int i = arraySize - 1; i > replaceIndex; i--)
    {
      array[i] = array[i - 1];
    }

    array[replaceIndex] = newValue;
  }
}
  

/* Old HPS below


int formantBins[5];

void formantHPS(int sampleWindowNumber)
{
  float hpsArray[FFT_WIN_SIZE / 2] = { 0 };
  int arraySizes[downsampleCount] = { 0 };
  float downsamples[downsampleCount][FFT_WIN_SIZE / 2] = { 0 };
  float formantFrequencies[FFT_WIN_SIZE / 4] = { 0 };

 /* for(int i = 0; i < downsampleCount; i++)
  {
    for(int k = 0; k < FFT_WIN_SIZE / 2; k++)
    {
      downsamples[i][k] = -1;
    }
  }

  //Downsample the magnitude spectrum a [downsampleCount] number of times times
  arraySizes[0] = doDownsample(downsamples[0], freqs[sampleWindowNumber], downsampleFactor);

  for(int i = 1; i < downsampleCount; i++)
  {
    arraySizes[i] = doDownsample(downsamples[i], downsamples[i-1], downsampleFactor);
  }

  //Find the product of all spectra arrays
  for(int i = 0; i < FFT_WIN_SIZE / 2; i++)
    {
      hpsArray[i] = freqs[sampleWindowNumber][i];
    }
  for(int i = 0; i < downsampleCount; i++)
  { 
    for(int k = 0; k < FFT_WIN_SIZE / 2; k++)
    {
      hpsArray[k] *= downsamples[i][k % arraySizes[i]];
    }
  }

  //Find peaks (formants) and categorize them into bins
  identifyPeaks(hpsArray, formantFrequencies);
  formantBinSort(formantBins, formantFrequencies);

  //Return the formants found in each frequency bin, and then do something to affect the output elsewhere
  //doStuffToOutput();
}

int doDownsample(float *downsampler, float *downsamplee, int DSfactor)
{
  int k = 0;

  for(int i = 0; i < FFT_WIN_SIZE / 2; i+= DSfactor)
  {
    downsampler[k] = downsamplee[i];
    k++;
  }

  return k;
}

void identifyPeaks(float *hpsArray, float *formantFrequencies)
{
  int k = 0;

  for(int i = 1; i < FFT_WIN_SIZE / 2; i++)
  {
    if(hpsArray[i] > hpsArray[i-1] && hpsArray[i] > hpsArray[i+1]) //May need to make this more sophisticated
    {
        formantFrequencies[k] = hpsArray[i];
        k++;
    }
  }
}

void formantBinSort(int *formantBins, float *formantFrequencies)
{
  //Do we need to account for all of the formants or just return a positive for each bin that has one?

  for (int i = 0; i < FFT_WIN_SIZE / 4; i++) //Can probably be made more efficient
  {
    if(formantFrequencies[i] > 0)
    {
      if(formantFrequencies[i] > averageBinsArray[0])
      {
         for(int k = 1; i < bins - 1; i++)
        {
          if(formantFrequencies[i] > averageBinsArray[i] && formantFrequencies[i] < averageBinsArray[i+1])
          {
            formantBins[i] = 1; //Setting a positive flag
          }
        }
        if(formantFrequencies[i] > averageBinsArray[bins-1])
        {
           formantBins[bins-1] = 1;
        }
      }
      else
      {
        formantBins[0] = 1;
      }
    }
  }
}
*/
