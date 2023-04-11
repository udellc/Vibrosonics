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

#define AUDIO_INPUT_PIN A2  // audio in pin (will use both A2 and A3 in the future for a stereo input)

#define SAMPLES_PER_SEC 32  // The number of sampling/analysis cycles per second, this MUST be a power of 2

#define FFT_SAMPLING_FREQ 10000                                                      // The sampling frequency, ideally should be a power of 2
//#define FFT_SAMPLING_FREQ_BASS 4096
#define FFT_WINDOW_SIZE int(pow(2, int(12 - (log(int(SAMPLES_PER_SEC)) / log(2)))))  // the FFT window size and number of audio samples for ideal performance, this MUST be a power of 2 \
                                                                                     // 2^(12 - log_base2(SAMPLES_PER_SEC)) = 64 for 64/s, 128 for 32/s, 256 for 16/s, 512 for 8/s

#define FFT_FLOOR_THRESH 300  // floor threshold for FFT, may be later changed to use the minimum from FFT

#define CONTROL_RATE int(pow(2, int(5 + (log(int(SAMPLES_PER_SEC)) / log(2)))))  // Update control cycles per second for ideal performance, this MUST be a power of 2 \
                                                                                 // 2^(5 + log_base2(SAMPLES_PER_SEC)) = 2048 for 64/s, 1024 for 32/s, 512 for 16/s, 256 for 8/s

#define OSCIL_COUNT 16  // The total number of waves to synthesize

#define DEFAULT_NUM_WAVES 5            // The default number of slices to take when the program runs
#define DEFAULT_BREADSLICER_CURVE 0.65  // The default curve for the breadslicer to follow when slicing

/*/
########################################################
    FFT
########################################################
/*/

const uint16_t FFT_WIN_SIZE = int(FFT_WINDOW_SIZE);
const float SAMPLE_FREQ = float(FFT_SAMPLING_FREQ);

const int FFT_WIN_SIZE_BY2 = int(FFT_WIN_SIZE) >> 1;

// Number of microseconds to wait between recording samples.
const int sampleDelayTime = ceil(1000000 / SAMPLE_FREQ);
const int totalSampleDelay = sampleDelayTime * FFT_WIN_SIZE;

int numSamplesTaken = 0;

float vReal[FFT_WIN_SIZE];  // vReal is used for input and receives computed results from FFT
float vImag[FFT_WIN_SIZE];  // vImag is used to store imaginary values for computation

int sumOfFFT = 0;

float vRealPrev[FFT_WIN_SIZE_BY2];
float maxAmpChange = 0;
float maxAmpChangeAmp = 0;
int freqMaxAmpChange = 0;

const float OUTLIER = 30000.0;  // Outlier for unusually high amplitudes in FFT

arduinoFFT FFT = arduinoFFT(vReal, vImag, FFT_WIN_SIZE, SAMPLE_FREQ);  // Object for performing FFT's

/*/
########################################################
    UPDATE RATE
########################################################
/*/
const int UPDATE_TIME = 1000000 / CONTROL_RATE;

int nextProcess = 0;                                                               // The next signal aquisition/processing phase to be completed in update_control
const int numSamplesPerProcess = floor(UPDATE_TIME / sampleDelayTime);            // The Number of samples to take per update
const int numProcessForSampling = (FFT_WIN_SIZE / numSamplesPerProcess) + 1;      // The number of update calls needed to sample audio
const int numTotalProcesses = numProcessForSampling + 2;                          // adding 2 more processes for FFT windowing function and other processing

const int SAMPLE_TIME = int(CONTROL_RATE / SAMPLES_PER_SEC);  // sampling and processing time in update_control
const int FADE_RATE = SAMPLE_TIME;                            // the number of cycles for fading
const float FADE_STEPX = float(2.0 / FADE_RATE);

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
int maxSumOfPeakAmps = 0;

const int FFT_AMP_SUM_THRESH = DEFAULT_NUM_WAVES * FFT_FLOOR_THRESH;

/*/
########################################################
    Stuff relevant to breadslicer function
########################################################
/*/

int slices = numWaves;  // accounts for how many slices the samples are split into
float amplitudeToRange = 250.0/10000.0;
float bySlices = 1.0 / numWaves;
float curve = float(DEFAULT_BREADSLICER_CURVE);  // curve accounts for the curve used when splitting the slices
int averageBinsArray[OSCIL_COUNT];

int sumOfAvgAmps = 0;  // sum of the average amplitudes for the current audio/FFT sample
int maxAvgAmp = 0;     // the maximum average amplitude, excluding outlier, throughout FFT
int ampSumMax = 0;

const float sFreqBy2 = SAMPLE_FREQ / 2;  // determine what frequency each amplitude represents
const float sFreqBySamples = float(sFreqBy2 / (FFT_WIN_SIZE >> 1));
const int BASS_FREQS_IDX = round(250.0 / sFreqBy2 * FFT_WIN_SIZE_BY2);

/*/
########################################################
    Values regarding audio input, which are used for signal processing
########################################################
/*/

int sampleAvg = 0;       // current sample average
int sampleAvgPrint = 0;  // the sample average to print to console
int currSampleMin = 5000;
int currSampleMax = 0;
int sampleMax = 0;               // running sample max
int sampleRange = 0;             // current sample range
int sampleRangeMin = 5000;       // running sample range min
int sampleRangeMax = 0;          // running sample range max, min and max values are used for detecting audio input accurately
int sampleRangeThreshold = 500;  // sample range threshold

int silenceTime = 0;                               // the total time the microphone input was minimal each increment of silenceTime is equal to (1 second / CONTROLRATE)
const int maxSilenceTime = 1 * int(CONTROL_RATE);  // the maximum time that the microphone can be mininal, used to silence the audio output after a certain time (5 seconds in this case)

unsigned long microseconds;

/*/
########################################################
    Stuff relavent to additive synthesizer
########################################################
/*/

EventDelay kChangeBinsDelay;
const unsigned int gainChangeMsec = 1000;

int totalNumWaves = int(DEFAULT_NUM_WAVES) << 1;

Oscil<2048, AUDIO_RATE> aSin[OSCIL_COUNT];

float amplitudeGains[OSCIL_COUNT];      // the amplitudes used for synthesis
int prevAmplitudeGains[OSCIL_COUNT];    // the previous amplitudes, used for curve smoothing
int nextAmplitudeGains[OSCIL_COUNT];    // the target amplitudes
int targetAmplitudeGains[OSCIL_COUNT];  // the target amplitudes
float ampGainStep[OSCIL_COUNT];         // the linear step values used for smoothing amplitude transitions

float waveFrequencies[OSCIL_COUNT];      // the frequencies used for synthesis
int nextWaveFrequencies[OSCIL_COUNT];    // the next frequencies
int targetWaveFrequencies[OSCIL_COUNT];  // the target frequencies
float waveFreqStep[OSCIL_COUNT];         // the linear step values used for smoothing frequency transitions

int toggleFreqChangeWaves = 0;  // toggle frequency transtions between next available waves and the previous waves, to change the frequencies of the next available waves

int8_t carrier;

int updateCount = 0;       // control rate cycle counter
int audioSampleCount = 0;  // sample and processing phases complete in control_rate cycle

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

  // setup reference tables for aSin[] oscillator array
  for (int i = 0; i < OSCIL_COUNT; i++) {
    aSin[i].setTable(SIN2048_DATA);
    amplitudeGains[i] = 0;
    targetAmplitudeGains[i] = 0;
    ampGainStep[i] = 0.0;
    waveFrequencies[i] = 0.0;
    targetWaveFrequencies[i] = 0;
    waveFreqStep[i] = 0.0;
    averageBinsArray[i] = 0;
  }

  for (int i = 0; i < FADE_RATE; i++) {
    float xStep = i * FADE_STEPX - 1.0;
    if (xStep < 0.0) {
      FADE_CONST[i] = -(pow(abs(xStep), 1.0) * 0.5 + 0.5);
    } else {
      FADE_CONST[i] = pow(xStep, 1.0) * 0.5 + 0.5;
    }
    Serial.println(FADE_CONST[i]);
  }

  // setup audio input pin
  pinMode(AUDIO_INPUT_PIN, INPUT);

  for (int i = 0; i < FFT_WIN_SIZE_BY2; i++) {
    vRealPrev[i] = 0.0;
    peakAmplitudes[i] = -1;
  }
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
    audioSampleCount = 0;
    nextProcess = 0;
  }
  //Serial.print(updateCount);

  /* Output string */
  //memset(outputString, 0, sizeof(outputString));

  int sampleTime = SAMPLE_TIME * audioSampleCount - 1;  // get the next available sample time
  // if it is time for next audio sample and analysis, complete the next processing phase
  if ((updateCount == sampleTime || updateCount == 0) && updateCount != CONTROL_RATE - 1) {
    fadeCounter = 0;
    nextProcess = 0;
    /* set number of slices and curve value */
    // while (Serial.available()) {
    //   char data = Serial.read();
    //   if (data == 'x') {
    //     changeNumBinsAndCurve();
    //   }
    // }
  }

  //unsigned long execT = micros();

  /* audio sampling phase */
  if (nextProcess < numProcessForSampling) {
    sampleAvg += recordSample(numSamplesPerProcess);
    nextProcess++;
    // Serial.print("audio sampling: ");
    // Serial.println(micros() - execT);
  }

  /* FFT phase*/
  else if (nextProcess == numTotalProcesses - 2) {
    FFT.DCRemoval();  // Remove DC component of signal
    //Serial.print("dc remvoal: ");
    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
    nextProcess++;
    // Serial.print("windowing: ");
    // Serial.println(micros() - execT);
  }
  /* next FFT and processing phase*/
  else if (nextProcess == numTotalProcesses - 1) {
    sampleAvgPrint = round(sampleAvg / FFT_WIN_SIZE);
    sampleAvg = 0;
    currSampleMin = 5000;
    currSampleMax = 0;

    /* Compute FFT */
    FFT.Compute(FFT_FORWARD);

    //Serial.print("FFT Cycle: ");

    /* Compute frequency magnitudes */
    FFT.ComplexToMagnitude();

    // Serial.print("FFT Cycle: ");
    // Serial.println(micros() - execT);

    /* Frequency of max amplitude change */

    /* Breadslicer */
    sumOfAvgAmps = breadslicer(vReal, slices, curve);

    //findMajorPeaks();

    //frequencyMaxAmplitudeDelta();
    // for (int i = 0; i <= numWaves; i++) {
    //   Serial.printf("(%d, %d),", peakIndexes[i], peakAmplitudes[i]);
    // }
    // Serial.println();

    //delay(500);

    /* mapping each average normalized bin from FFT analysis to a sine wave gain value for additive synthesis */
    mapAmplitudes();
    // for (int i = 0; i < numWaves; i++) {
    //   Serial.printf("(%03d, %03d), ", targetWaveFrequencies[i], targetAmplitudeGains[i]);
    // }
    // Serial.println();

    // increment sample count and reset next process
    audioSampleCount++;
    nextProcess++;
  }
  //Serial.printf("%03d, %03d, %03d\n", updateCount, fadeCounter, nextProcess);
  if (fadeCounter < FADE_RATE) {
    crossfadeAmpsFreqs();
    // Serial.print("fading: ");
    // Serial.println(micros() - execT);
  }

  /* Serial Ouput */
  // char buffer3[128];
  //Serial.printf("Plotter: Min:%d\tMax:%d\tOutput(0-255):%03d\tInput(0-255):%03d\t\n", 0, 255, map(carrier, -127, 128, 0, 255), map(sampleAvgPrint, 0, sampleMax, 0, 255));
  // strcat(outputString, buffer3);

  // increment update_control calls counter
  updateCount++;
  //Serial.print(outputString);
}


void mapAmplitudes() {
  // sumOfPeakAmps = 0;
  // int sumOfAmps = 0;
  // for (int i = 0; i < numWaves; i++) {
  //   if (peakAmplitudes[i] != -1 || peakAmplitudes[i] > 500) {
  //     sumOfPeakAmps += peakAmplitudes[i];
  //   } else {
  //     peakAmplitudes[i] = -1;
  //   }
  //   if (averageBinsArray[i] > 500) {
  //     sumOfAmps += averageBinsArray[i];
  //   } else {
  //     averageBinsArray[i] = 0;
  //   }
  // }
  // if (sumOfPeakAmps > maxSumOfPeakAmps) {
  //   maxSumOfPeakAmps = sumOfPeakAmps;
  // }

  int aLocation = 0;
  int aLocationMax = numWaves;
  if (toggleFreqChangeWaves == 1) {
    aLocation = numWaves;
    aLocationMax = totalNumWaves;
  }
  for (int i = 0; i < totalNumWaves; i++) {
    prevAmplitudeGains[i] = nextAmplitudeGains[i];
    nextAmplitudeGains[i] = targetAmplitudeGains[i];
    // if (sumOfFFT < FFT_AMP_SUM_THRESH) {
    //   targetAmplitudeGains[i] = targetAmplitudeGains[i] >> 1;
    // } else {
    // otherwise map amplitude -> 0-255
    int gainValue = 0;
    int frequency = 0;
    // if iterator is on one of the next available waves, map its amplitude, otherwise gain value is 0
    if (i >= aLocation && i < aLocationMax) {
      gainValue = averageBinsArray[i - aLocation];
      if (gainValue > 0) {
        //gainValue = map(gainValue, 0, sumOfAmps, 0, 255);
        gainValue = round(gainValue * bySlices * amplitudeToRange);
        frequency = targetWaveFrequencies[i - aLocation];
        aSin[i].setFreq(frequency);
      }
      // if (peakAmplitudes[i - aLocation] > 0) {
      //   gainValue = map(peakAmplitudes[i - aLocation], 0, sumOfPeakAmps, 0, 255);
      //   float targetFrequency = peakIndexes[i - aLocation];
      //   // if (targetFrequency <= BASS_FREQS_IDX) {
      //   //   aSin[i].setFreq(int(map(targetFrequency, 1, BASS_FREQS_IDX, 20, 200)));
      //   // }
      //   float normalizedFreq = float(targetFrequency / (FFT_WIN_SIZE_BY2 - 1));
      //   normalizedFreq = pow(normalizedFreq, 0.8);
      //   int targetFreq = int(round(normalizedFreq * 184) + 16);
      //   targetWaveFrequencies[i - aLocation] = targetFreq;
      // }
      // frequency = targetWaveFrequencies[i - aLocation];
      // aSin[i].setFreq(frequency);
      //}
      //Serial.printf("(%d, %d, %03d, %03d)\t", i, i - aLocation, gainValue, frequency);
      targetAmplitudeGains[i] = gainValue;
    }
    // calculate the value to alter the amplitude per each update cycle
    // if (FADE_RATE > 1) {
    //   ampGainStep[i] = float(nextAmplitudeGains[i] - prevAmplitudeGains[i]) / FADE_RATE;
    // } else {
    //   ampGainStep[i] = float(nextAmplitudeGains[i] - prevAmplitudeGains[i]);
    // }
    ampGainStep[i] = float(nextAmplitudeGains[i] - prevAmplitudeGains[i]);

    // form output fstring
    //formBinsArrayString(amplitudeGains[i], waveFrequencies[i], slices, i, curve, ampGainStep[i]);
  }
  //Serial.println();
  toggleFreqChangeWaves = 1 - toggleFreqChangeWaves;
}

void crossfadeAmpsFreqs() {
  for (int i = 0; i < totalNumWaves; i++) {
    // if (toggleFreqChangeWaves == 1) {
    //   if (i < numWaves) {
    //     amplitudeGains[i] = round(prevAmplitudeGains[i] + FADE_CONST[fadeCounter] * ampGainStep[i]);
    //   } else if (fadeCounter < (FADE_RATE >> 1)) {
    //     amplitudeGains[i] = round(prevAmplitudeGains[i] + FADE_CONST[fadeCounter * 2] * ampGainStep[i]);
    //   }
    // } else {
    //   if (i >= numWaves) {
    //     amplitudeGains[i] = round(prevAmplitudeGains[i] + FADE_CONST[fadeCounter] * ampGainStep[i]);
    //   } else if (fadeCounter < (FADE_RATE >> 1)) {
    //     amplitudeGains[i] = round(prevAmplitudeGains[i] + FADE_CONST[fadeCounter * 2] * ampGainStep[i]);
    //   }
    // }
    //amplitudeGains[i] = round(prevAmplitudeGains[i] + FADE_CONST[fadeCounter] * ampGainStep[i]);
    // smoothen the transition between each amplitude, checking for difference to avoid additonal smoothing
    if (abs(nextAmplitudeGains[i] - int(round(amplitudeGains[i]))) > 2) {
      if (i >= (toggleFreqChangeWaves * numWaves) && i < ((toggleFreqChangeWaves + 1) * numWaves)) {
        amplitudeGains[i] = round(prevAmplitudeGains[i] + FADE_CONST[fadeCounter] * ampGainStep[i]);
      } else {
        amplitudeGains[i] = round(prevAmplitudeGains[i] + FADE_CONST[int(round(fadeCounter * 1.5))] * ampGainStep[i]);
      }
    } else {
      amplitudeGains[i] = nextAmplitudeGains[i];
    }
    // transition between each frequency
    //waveFrequencies[i] = nextWaveFrequencies[i];

    //aSin[i].setFreq(int(waveFrequencies[i]));
    //Serial.printf("(%.2f, %03d, %03d), ", amplitudeGains[i], nextAmplitudeGains[i], targetAmplitudeGains[i]);
  }
  fadeCounter++;
  //toggleWaveStep = 1 - toggleWaveStep;
  //Serial.println();
}

AudioOutput_t updateAudio() {
  int longCarrier = 0;

  for (int i = 0; i < totalNumWaves; i++) {
    longCarrier += amplitudeGains[i] * aSin[i].next();
    // if (amplitudeGains[i] != 0) {
    //   longCarrier += amplitudeGains[i] * aSin[i].next();
    // }
  }
  carrier = int8_t(longCarrier >> 8);

  return carrier;
}

/*/
########################################################
    FTT processing
########################################################
/*/

void changeNumBinsAndCurve() {
  Serial.println("Enter integer for number of slices:");
  while (Serial.available() == 1) {}
  slices = Serial.parseInt();
  Serial.println("Enter float for curve value:");
  while (Serial.available() == 1) {}
  curve = Serial.parseFloat();
  Serial.println("New slices and curve value set.");
  Serial.printf("slices: %d\tcurve: %.2f\n", slices, curve);
  delay(500);
}

/* Splits the bufferSize into X groups, where X = sliceInto
    the curveValue determines the curve to follow when buffer is split
    if curveValue = 1 : then buffer is sliced into X even groups
    0 < curveValue < 1 : then buffer is sliced into X groups, following a concave curve
    1 < curveValue < infinity : then buffer is sliced into X groups following a convex curve */
int breadslicer(float *data, int sliceInto, float curveValue) {
  float eq = 0.25 / (FFT_WIN_SIZE_BY2 - 1);
  // The parabolic curve: (x/sliceInto)^(1/curveValue)
  float step = 1.0 / sliceInto;       // how often to step on the x-axis to determine bin value
  float exponent = 1.0 / (step + curveValue);  // power of the parabolic curve (x^exponent)

  sumOfFFT = 0;
  int curMaxAvg = 2000;
  int ampsSum = 0;
  float topOfSample = sFreqBySamples;  // The frequency of the current amplitude

  int lastJ = 1;                         // the last amplitude taken from vReal
  for (int i = 0; i < sliceInto; i++) {  // Calculate the size of each bin and the amplitudes into each bin
    float xStep = (i + 1) * step;        // x-axis step
    // (x/sliceInto)^(1/curveValue)
    int binSize = round(FFT_WIN_SIZE_BY2 * pow(xStep, exponent));
    int newJ = lastJ;
    int ampGroupSum = 0;
    int ampGroupCount = 0;
    int maxSliceAmp = 0;
    int maxSliceAmpFreq = 0;
    int averageFreq = 0;
    for (int j = lastJ; j < binSize; j++) {        // for the next group of amplitudes
      topOfSample += sFreqBySamples;  // calculate the associated frequency
      // if amplitude is above certain threshold, set it to -1 to exclude from signal processing
      int amp = round(data[j]);// * (0.75 + eq * j));
      if (amp > FFT_FLOOR_THRESH && amp < OUTLIER) {
        amp -= FFT_FLOOR_THRESH;
        sumOfFFT += amp;
        ampGroupSum += amp;
        averageFreq += topOfSample;
      } else {
        amp = 0;
      }
      if (amp > maxSliceAmp) {
        maxSliceAmp = amp;
        maxSliceAmpFreq = int(round(topOfSample));
      }
      ampGroupCount += 1;
      // Debugging stuff
      // Serial.printf("I: %d\tFreq: %.2f\tAmp: %.2f\t", j, topOfSample, data[j]);
      // Serial.println();
      newJ += 1;
    }
    //if the current slice contains at least two frequencies, then assign the average, otherwise assign the sum of the slice
    if (ampGroupCount > 1) {
      averageBinsArray[i] = int(round(ampGroupSum / ampGroupCount));
      //averageFreq = int(round(averageFreq / ampGroupCount));
    } else {
      averageBinsArray[i] = ampGroupSum;
    }
    //Serial.println(averageBinsArray[i]);
    // store the prev target gains
    // if there is at least one amplitude in the group, map it's frequency, otherwise frequency for that slice is unchanged
    if (ampGroupCount > 0) {
      if (maxSliceAmpFreq < 150) {
        targetWaveFrequencies[i] = maxSliceAmpFreq * 0.5;
      }
      else if (maxSliceAmpFreq < 300) {
        targetWaveFrequencies[i] = maxSliceAmpFreq * 0.5;
      } else {
        float normalizedFreq = float(maxSliceAmpFreq) / sFreqBy2;
        normalizedFreq = pow(normalizedFreq, 1.0);
        int targetFreq = int(round(normalizedFreq * 120) + 60);
        //int targetFreq = map(maxSliceAmpFreq, 0, int(sFreqBy2), 20, 200);
        targetWaveFrequencies[i] = targetFreq;
      }
    }
    //averageBinsArray[i] = maxSliceAmp;

    ampsSum += averageBinsArray[i];
    if (maxSliceAmp > curMaxAvg) {
        curMaxAvg = maxSliceAmp;
    }
    // if (averageBinsArray[i] > maxAvgAmp) {
    //   maxAvgAmp = averageBinsArray[i];
    //   if (maxSliceAmp > 2500) {
    //     amplitudeToRange = 255.0 / maxSliceAmp;
    //   }
    // }
    lastJ = newJ;
  }
  amplitudeToRange = float(250.0 / curMaxAvg);
  return ampsSum;
}

/* find all the major peaks in the fft spectrogram, based on the threshold and how many peaks to find */
void findMajorPeaks() {
  sumOfFFT = 0;
  int majorPeaksCounter = 0;
  int peakReached = 0;
  // traverse through spectrogram and find all peaks (above FFT_FLOOR_THRESH)
  for (int i = 1; i < FFT_WIN_SIZE_BY2; i++) {
    // skip the iteration if the fft amp is an unsually high value to exclude them from calculations (usually occurs on the lowest and highest frequencies)
    if (vReal[i - 1] > OUTLIER) {
      continue;
    }
    sumOfFFT += vReal[i - 1];
    // get the change between the next and last index
    float change = vReal[i] - vReal[i - 1];
    // if change is positive, consider that the peak hasn't been reached yet
    if (change > 0.0) {
      peakReached = 0;
      // otherwise if change is 0 or negative, consider this the peak
    } else {
      // waiting for next positive change, to start looking for peak
      if (peakReached == 0) {
        // if the amplitude is higher then a certain threshold, consider a peak has been reached
        if (vReal[i - 1] > FFT_FLOOR_THRESH) {
          // print all considered peaks so far
          //Serial.printf("(%d,%d),", i - 1, int(round(vReal[i - 1])));
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
              if (peakAmplitudes[j] < vReal[i - 1]) {
                lowestPeakIndex = j;
              }
            }
            // if the lowest peak is the lastPeak then check if the last lowest peak was lower
            if (lowestPeakIndex == majorPeaksCounter - 1) {
              if (peakAmplitudes[lowestPeakIndex] < vReal[i - 1]) {
                peakAmplitudes[lowestPeakIndex] = vReal[i - 1];
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
          peakAmplitudes[majorPeaksCounter - 1] = int(round(vReal[i - 1]));
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


// frequency of max amplitude change
void frequencyMaxAmplitudeDelta() {
  maxAmpChange = 0;
  int maxAmpChangeI = 0;
  for (int i = 0; i < FFT_WIN_SIZE_BY2; i++) {
    if (vReal[i] < OUTLIER) {
      int currAmpChange = abs(vReal[i] - vRealPrev[i]);
      if (currAmpChange > maxAmpChange) {
        maxAmpChange = currAmpChange;
        maxAmpChangeI = i;
      }
    }
    vRealPrev[i] = vReal[i];
  }
  peakAmplitudes[numWaves] = map(vReal[maxAmpChangeI], 0, sumOfPeakAmps, 0, 255);

  freqMaxAmpChange = int(round((maxAmpChangeI + 1) * sFreqBySamples));
  targetWaveFrequencies[numWaves] = map(freqMaxAmpChange, 0, int(sFreqBy2), 16, 200);
}

// form slices array string
void formBinsArrayString(int binValue, int frequency, int arraySize, int i, float curveValue, int gainStep) {
  float step = 1.0 / arraySize;
  float parabolicCurve = 1.0 / curveValue;

  float xStep = i * step;
  char buffer[48];
  memset(buffer, 0, sizeof(buffer));
  int rangeLow = 0;
  if (i > 0) {
    rangeLow = round(sFreqBy2 * pow(xStep, parabolicCurve));
  }
  int rangeHigh = round(sFreqBy2 * pow(xStep + step, parabolicCurve));
  sprintf(buffer, "%d - %dHz: %03d for %03dHz, %03d\t", rangeLow, rangeHigh, binValue, frequency, gainStep);
  strcat(outputString, buffer);
}

// Records samples into the vReal and samples[] array from the audio input pin, at a sampling frequency of SAMPLE_FREQ
int recordSample(int sampleCount) {
  if (sampleCount > 0 && numSamplesTaken < FFT_WIN_SIZE) {
    //Serial.println(numSamplesTaken);
    long int sampleTime = micros();
    int i = numSamplesTaken;
    int sample;
    sample = mozziAnalogRead(AUDIO_INPUT_PIN);  // Read sample from input
    vReal[i] = float(sample);  // For FFT to Bins code
    vImag[i] = 0.0;              // set imaginary values to 0
    while (micros() - sampleTime < sampleDelayTime) {}
    numSamplesTaken++;
    return sample + recordSample(sampleCount - 1);
  } else if (numSamplesTaken > FFT_WIN_SIZE - 1) {
    numSamplesTaken = 0;
  }
  return 0;
}

/*/
########################################################
    Formant Estimation 
########################################################
/*/
//Downsample x times, and then compare peaks in each window, if there are matching peaks, throw a positive for a formant

static int downsampleCount = 5;  //How many times to downsample. More generally gives higher precision, but requires more processing power. Refers to number of harmonics being considered.
//int downsampleFactor = 2; //Factor to downsample by. Setting to 2 means we're picking every other data point when downsampling.
static int lpcOrder = 10;

int maleFormantTable[6];


float thisWindow[FFT_WIN_SIZE / 2];

void crudeFindFormants() {
  maleFormantTable[0] = 2160;  // i
  maleFormantTable[2] = 1865;  // y
  maleFormantTable[3] = 1920;  // e
  maleFormantTable[3] = 760;   // a
  maleFormantTable[4] = 280;   // o
  maleFormantTable[5] = 340;   // u

  int maxAmpOne = 0;
  int maxAmpTwo = 0;
  int strongFreqOne = 0;
  int strongFreqTwo = 0;
  //Potentially add a 3rd to account for fundemental freq.

  for (int i = 0; i < FFT_WIN_SIZE / 2; i++) {
    Serial.printf("%d\n", vReal[i]);
    if (vReal[i] > 100) {
      if (vReal[i] > maxAmpOne) {
        maxAmpOne = vReal[i];
        strongFreqOne = i;
        continue;
      } else if (vReal[i] > strongFreqTwo) {
        maxAmpTwo = vReal[i];
        strongFreqTwo = i;
      }
    }
  }

  for (int i = 0; i < 6; i++) {
    if (abs(strongFreqOne - strongFreqTwo) > i - i * 0.08 && abs(strongFreqOne - strongFreqTwo) < maleFormantTable[i] + 30) {
      //Serial.printf("Formant Found! 1-%d | 2-%d | Difference - %d\n");
    } else {
      //Serial.printf("Formant not Found 1-%d | 2-%d | Difference - %d\n");
    }
  }
}

void findFormants() {
  for (int i = 0; i < FFT_WIN_SIZE / 2; i++) {
    thisWindow[i] = vReal[i];
  }

  for (int i = 0; i < downsampleCount; i++) {
  }
}

void autoCorrelation() {
  for (int i = 0; i < lpcOrder; i++) {
    for (int k = 0; k < FFT_WIN_SIZE / 2; k++) {
    }
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
/*
if in 50 - 90 hz range, amplitude > number, detect kick bass?
if in 50 - 90 hz range, amplitude > number, if repeated pattern, detect kick bass?
*/