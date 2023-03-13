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

#define AUDIO_INPUT_PIN A2          // audio in pin (will use both A2 and A3 in the future for a stereo input)

#define SAMPLES_PER_SEC 16           // The number of sampling/analysis cycles per second, this MUST be a power of 2

#define FFT_SAMPLING_FREQ 8192     // The sampling frequency, ideally should be a power of 2
#define FFT_WINDOW_SIZE int(pow(2, int(12 - (log(int(SAMPLES_PER_SEC)) / log(2)))))  // the FFT window size and number of audio samples for ideal performance, this MUST be a power of 2
                        // 2^(12 - log_base2(SAMPLES_PER_SEC)) = 64 for 64/s, 128 for 32/s, 256 for 16/s, 512 for 8/s

#define FFT_FLOOR_THRESH 1000       // floor threshold for FFT, may be later changed to use the minimum from FFT

#define CONTROL_RATE int(pow(2, int(4 + (log(int(SAMPLES_PER_SEC)) / log(2)))))     // Update control cycles per second for ideal performance, this MUST be a power of 2
                        // 2^(5 + log_base2(SAMPLES_PER_SEC)) = 2048 for 64/s, 1024 for 32/s, 512 for 16/s, 256 for 8/s

#define OSCIL_COUNT 16              // The total number of waves to synthesize

#define DEFAULT_BREADSLICES 5            // The default number of slices to take when the program runs
#define DEFAULT_BREADSLICER_CURVE 0.8     // The default curve for the breadslicer to follow when slicing

/*/
########################################################
    FFT
########################################################
/*/

const uint16_t FFT_WIN_SIZE = int(FFT_WINDOW_SIZE);
const float SAMPLE_FREQ = float(FFT_SAMPLING_FREQ);

// Number of microseconds to wait between recording samples.
const int sampleDelayTime = ceil(1000000 / SAMPLE_FREQ);
const int totalSampleDelay = sampleDelayTime * FFT_WIN_SIZE;

int numSamplesTaken = 0;

float vReal[FFT_WIN_SIZE];          // vReal is used for input and receives computed results from FFT
float vImag[FFT_WIN_SIZE];          // vImag is used to store imaginary values for computation

float vRealPrev[FFT_WIN_SIZE];
float maxAmpChange = 0;
float maxAmpChangeAmp = 0;
int freqMaxAmpChange = 0;

const float OUTLIER = 30000.0;     // Outlier for unusually high amplitudes in FFT

arduinoFFT FFT = arduinoFFT(vReal, vImag, FFT_WIN_SIZE, SAMPLE_FREQ); // Object for performing FFT's

/*/
########################################################
    UPDATE RATE
########################################################
/*/
const int UPDATE_TIME = ceil(1000000 / CONTROL_RATE);

int nextProcess = 0;                                                         // The next signal aquisition/processing phase to be completed in update_control 
const int numProcessForSampling = ceil(totalSampleDelay / UPDATE_TIME);   // The number of update calls needed to sample audio
const int numSamplesPerProcess = floor(FFT_WIN_SIZE / numProcessForSampling) + 1; // The Number of samples to take per process
const int numTotalProcesses = numProcessForSampling + 2;                          // adding 2 more processes for FFT windowing function and other processing

const int SAMPLE_TIME = int(CONTROL_RATE / SAMPLES_PER_SEC);       // sampling and processing time in update_control
const int FADE_RATE = int(SAMPLE_TIME) >> 1 - 1;

int fadeCounter = 0;

/*/
########################################################
    Stuff relevant to calculate frequency peaks of an FFT spectrogram
########################################################
/*/
int peakIndexes[FFT_WIN_SIZE >> 1];
int peakAmplitudes[FFT_WIN_SIZE >> 1];

int numPeaks = 8;
int sumOfPeakAmps = 0;
int maxSumOfPeakAmps = 0;

int FFT_AMP_SUM_THRESH = numPeaks * 3000;

/*/
########################################################
    Stuff relevant to breadslicer function
########################################################
/*/

int slices = int(DEFAULT_BREADSLICES);                       // accounts for how many slices the samples are split into
float curve = float(DEFAULT_BREADSLICER_CURVE);                 // curve accounts for the curve used when splitting the slices
int averageBinsArray[OSCIL_COUNT];

int sumOfAvgAmps = 0;               // sum of the average amplitudes for the current audio/FFT sample
int maxAvgAmp = 0;                  // the maximum average amplitude, excluding outlier, throughout FFT
int ampSumMax = 0;

const float sFreqBy2 = SAMPLE_FREQ / 2;  // determine what frequency each amplitude represents
const float sFreqBySamples = float(sFreqBy2 / (FFT_WIN_SIZE >> 1));

/*/
########################################################
    Values regarding audio input, which are used for signal processing
########################################################
/*/

int sampleAvg = 0;              // current sample average
int sampleAvgPrint = 0;         // the sample average to print to console
int currSampleMin = 5000;
int currSampleMax = 0;
int sampleMax = 0;              // running sample max
int sampleRange = 0;            // current sample range
int sampleRangeMin = 5000;      // running sample range min
int sampleRangeMax = 0;         // running sample range max, min and max values are used for detecting audio input accurately
int sampleRangeThreshold = 500;   // sample range threshold 

int silenceTime = 0;                                  // the total time the microphone input was minimal each increment of silenceTime is equal to (1 second / CONTROLRATE)
const int maxSilenceTime = 1 * int(CONTROL_RATE);     // the maximum time that the microphone can be mininal, used to silence the audio output after a certain time (5 seconds in this case)

unsigned long microseconds;

/*/
########################################################
    Stuff relavent to additive synthesizer
########################################################
/*/

EventDelay kChangeBinsDelay;
const unsigned int gainChangeMsec = 1000;

// Oscil <2048, AUDIO_RATE> asinc(SIN2048_DATA); //asinc is the carrier wave 20Hz. This is the sum of all other waves and the wave that should be outputted.
// Oscil <2048, AUDIO_RATE> asin1(SIN2048_DATA); //20Hz
// Oscil <2048, AUDIO_RATE> asin2(SIN2048_DATA); //30 - 39
// Oscil <2048, AUDIO_RATE> asin3(SIN2048_DATA); //40 - 49
// Oscil <2048, AUDIO_RATE> asin4(SIN2048_DATA); //50 - 59
//Oscil <2048, AUDIO_RATE> asin5(SIN2048_DATA); //60 - 69 // These are added for later, when changing the number of slices
//Oscil <2048, AUDIO_RATE> aVibrato(COS2048_DATA);

Oscil <2048, AUDIO_RATE> aSin[OSCIL_COUNT];


float amplitudeGains[OSCIL_COUNT];     // the amplitudes used for synthesis
int nextAmplitudeGains[OSCIL_COUNT];  // the target amplitudes
int targetAmplitudeGains[OSCIL_COUNT];  // the target amplitudes
float ampGainStep[OSCIL_COUNT];        // the linear step values used for smoothing amplitude transitions

float waveFrequencies[OSCIL_COUNT];    // the frequencies used for synthesis
int nextWaveFrequencies[OSCIL_COUNT];  // the next frequencies
int targetWaveFrequencies[OSCIL_COUNT];// the target frequencies
float waveFreqStep[OSCIL_COUNT];       // the linear step values used for smoothing frequency transitions

int toggleWaveStep = 0;               // toggle wave step transtions (setFreq()) to reduce processing load

int8_t carrier;

int updateCount = 0;                  // control rate cycle counter
int audioSampleCount = 0;                  // sample and processing phases complete in control_rate cycle

int domFreq = 0;                      // nanolux domFreq

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
  for(int i = 0; i < OSCIL_COUNT; i++)
  {
    aSin[i].setTable(SIN2048_DATA);
    amplitudeGains[i] = 0.0;
    targetAmplitudeGains[i] = 0;
    ampGainStep[i] = 0.0;
    waveFrequencies[i] = 0.0;
    targetWaveFrequencies[i] = 0;
    waveFreqStep[i] = 0.0;
    averageBinsArray[i] = 0;
  }
  
  // setup audio input pin
  pinMode(AUDIO_INPUT_PIN, INPUT);

  for (int i = 0; i < FFT_WIN_SIZE; i++) {
    vRealPrev[i] = 0.0;
  }
  // Additive Synthesis
  //aVibrato.setFreq(4.f);
  //asind.setFreq(0);  //Nanolux dominant frequency value changes when enough samples are taken
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
  if (updateCount > CONTROL_RATE - 1) {
    updateCount = 0;
    audioSampleCount = 0;
    nextProcess = 0;
  }

  /* Output string */
  //memset(outputString, 0, sizeof(outputString));

  int sampleTime = SAMPLE_TIME * audioSampleCount;   // get the next available sample time
  // if it is time for next audio sample and analysis, complete the next processing phase
  if (updateCount == sampleTime + nextProcess) {
    fadeCounter = 0;
    
    /* set number of slices and curve value */
    // while (Serial.available()) {
    //   char data = Serial.read();
    //   if (data == 'x') {
    //     changeNumBinsAndCurve();
    //   }
    // }

    //unsigned long execT = micros();

    /* audio sampling phase */
    if (nextProcess < numProcessForSampling) {
      sampleAvg += recordSample(numSamplesPerProcess);
      nextProcess++;
      // Serial.print("audio sampling: ");
    }

    /* FFT phase*/
    else if (nextProcess == numTotalProcesses - 2) {
      // FFT.DCRemoval();                                // Remove DC component of signal
      FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
      nextProcess++;
      // Serial.print("windowing: ");
    }
    /* next FFT and processing phase*/
    else if (nextProcess == numTotalProcesses - 1) {
      sampleAvgPrint = round(sampleAvg / FFT_WIN_SIZE);
      sampleAvg = 0;
      currSampleMin = 5000;
      currSampleMax = 0;

      /* Compute FFT */
      FFT.Compute(FFT_FORWARD);
      
      /* Compute frequency magnitudes */
      FFT.ComplexToMagnitude();
 
      /* Frequency of max amplitude change */
      
      /* Breadslicer */
      //sumOfAvgAmps = breadslicer(vReal, FFT_WIN_SIZE >> 1, slices, curve);
      findMajorPeaks();

      frequencyMaxAmplitudeDelta();
      // for (int i = 0; i < numPeaks; i++) {
      //   Serial.printf("(%d, %d),", peakIndexes[i], peakAmplitudes[i]);
      // }
      // Serial.println();

      //delay(500);

      /* mapping each average normalized bin from FFT analysis to a sine wave gain value for additive synthesis */
      //mapAmplitudes();
      mapFreqAmplitudes();
      
      // increment sample count and reset next process
      audioSampleCount++;
      nextProcess = 0;
      // Serial.print("analysis: ");
    }

    // Serial.println(micros() - execT);
  }
  if (fadeCounter < FADE_RATE) {
    crossfadeAmpsFreqs();
  }

  /* Serial Ouput */
  // char buffer3[128];
  // Serial.printf("Plotter: Min:%d\tMax:%d\tOutput(0-255):%03d\tInput(0-255):%03d\t\n", 0, 255, map(carrier, -127, 128, 0, 255), map(sampleAvgPrint, 0, sampleMax, 0, 255));
  // strcat(outputString, buffer3);

  // increment update_control calls counter
  updateCount++;
  //Serial.print(outputString);
}

void mapFreqAmplitudes() {
  sumOfPeakAmps = 0;
  for (int i = 0; i <= numPeaks; i++) {
    if (peakAmplitudes[i] == -1) {
      break;
    }
    sumOfPeakAmps += peakAmplitudes[i];
  }
  if (sumOfPeakAmps > maxSumOfPeakAmps) {
    maxSumOfPeakAmps = sumOfPeakAmps;
  }
  for (int i = 0; i <= numPeaks; i++) {
    nextAmplitudeGains[i] = targetAmplitudeGains[i];
    nextWaveFrequencies[i] = targetWaveFrequencies[i];
    if (peakAmplitudes[i] == -1 || sumOfPeakAmps < FFT_AMP_SUM_THRESH) {
      targetAmplitudeGains[i] = 0;
    } else {
      targetAmplitudeGains[i] = map(peakAmplitudes[i], 0, sumOfPeakAmps, 1, 255);
      int targetFrequency = (peakIndexes[i] + 1) * sFreqBySamples;
      targetWaveFrequencies[i] = map(targetFrequency, 0, sFreqBy2, 20, 200);
    }
    if (FADE_RATE > 1) {
      waveFreqStep[i] = float((nextWaveFrequencies[i] - waveFrequencies[i]) / FADE_RATE);
      ampGainStep[i] = float((nextAmplitudeGains[i] - amplitudeGains[i]) / FADE_RATE);
    } else {
      waveFreqStep[i] = float(nextWaveFrequencies[i] - waveFrequencies[i]);
      ampGainStep[i] = float(nextAmplitudeGains[i] - amplitudeGains[i]);
    }
  }
}

void mapAmplitudes() {
  for (int i = 0; i < slices; i++) {
    if (sampleRange < sampleRangeThreshold) {
      silenceTime += 1;
      if (silenceTime >= int(CONTROL_RATE)) {
        targetAmplitudeGains[i] = 0;
        silenceTime = maxSilenceTime;
      } else if (targetAmplitudeGains[i] >= 25) {
        targetAmplitudeGains[i] -= 25;
      } else if (targetAmplitudeGains[i] > 1) {
        targetAmplitudeGains[i] = targetAmplitudeGains[i] >> 1;
      }
    } else {
      silenceTime = 0;
      // otherwise map amplitude -> 0-255
      int gainValue = map(averageBinsArray[i], 0, sumOfAvgAmps, 0, 255);
      //totalGains += gainValue;
      targetAmplitudeGains[i] = gainValue;
    }
    // calculate the value to alter the amplitude per each update cycle
    if (FADE_RATE > 1) {
      ampGainStep[i] = float((nextAmplitudeGains[i] - amplitudeGains[i]) / FADE_RATE);
    } else {
      ampGainStep[i] = float(nextAmplitudeGains[i] - amplitudeGains[i]);
    }

  // form output fstring
  //formBinsArrayString(amplitudeGains[i], waveFrequencies[i], slices, i, curve, ampGainStep[i]);
  }
}

void crossfadeAmpsFreqs() {
  for(int i = 0; i <= numPeaks; i++) {
    // smoothen the transition between each amplitude, checking for difference to avoid additonal smoothing
    if (abs(nextAmplitudeGains[i] - int(round(amplitudeGains[i]))) > 1) {
      amplitudeGains[i] += ampGainStep[i];
    } else {
      amplitudeGains[i] = nextAmplitudeGains[i];
    }
    // smoothen the transition between each frequency
    if (abs(nextWaveFrequencies[i] - int(round(waveFrequencies[i]))) > 1) {
      waveFrequencies[i] = round(waveFrequencies[i] + waveFreqStep[i]);
    } else {
      waveFrequencies[i] = nextWaveFrequencies[i];
    }

    aSin[i].setFreq(int(waveFrequencies[i]));
  }
  //toggleWaveStep = 1 - toggleWaveStep;
  fadeCounter++;
}

AudioOutput_t updateAudio() {
  //Q15n16 vibrato = (Q15n16) aVibrato.next();
  int longCarrier = 0;

  for(int i = 0; i < numPeaks; i++)
  {
    longCarrier += int(amplitudeGains[i]) * aSin[i].next();
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
int breadslicer(float *data, uint16_t bufferSize, int sliceInto, float curveValue) {            
  // The parabolic curve: (x/sliceInto)^(1/curveValue)
  float step = 1.0 / sliceInto;                         // how often to step on the x-axis to determine bin value
  float exponent = 1.0 / curveValue;                    // power of the parabolic curve (x^exponent)

  int avgAmpsSum = 0;
  float topOfSample = 0;                                // The frequency of the current amplitude

  int lastJ = 0;                                        // the last amplitude taken from vReal
  for (int i = 0; i < sliceInto; i++) {                 // Calculate the size of each bin and the amplitudes into each bin
    float xStep = (i + 1) * step;                       // x-axis step
    // (x/sliceInto)^(1/curveValue)
    int binSize = round(bufferSize * pow(xStep, exponent));
    int newJ = lastJ;
    int ampGroupSum = 0;
    int ampGroupCount = 0;
    int maxBinAmp = 0;
    int maxBinAmpFreq = 0;
    int averageFreq = 0;
    for (int j = lastJ; j < binSize; j++) {             // for the next group of amplitudes
      topOfSample = topOfSample + sFreqBySamples;       // calculate the associated frequency
      // if amplitude is above certain threshold, set it to -1 to exclude from signal processing
      if (data[j] > FFT_FLOOR_THRESH && data[j] < OUTLIER) {
        int amp = int(data[j]);
        ampGroupSum += amp;
        ampGroupCount += 1;
        averageFreq += topOfSample;
        if (amp > maxBinAmp) {
          maxBinAmp = amp;
          maxBinAmpFreq = int(topOfSample);
        }
      }
      // Debugging stuff
      // Serial.printf("I: %d\tFreq: %.2f\tAmp: %.2f\t", j, topOfSample, vData[j]);
      // Serial.println();
      newJ += 1;
    }
    // if the current slice contains at least two frequencies, then assign the average, otherwise assign the sum of the slice
    if (ampGroupCount > 1) {
      averageBinsArray[i] = int(round(ampGroupSum / ampGroupCount));
      averageFreq = int(round(averageFreq / ampGroupCount));
    } else {
      averageBinsArray[i] = ampGroupSum;
    }
    // store the prev target gains
    nextAmplitudeGains[i] = targetAmplitudeGains[i];
    nextWaveFrequencies[i] = targetWaveFrequencies[i];
    // if there is at least one amplitude in the group, map it's frequency, otherwise frequency for that slice is unchanged
    if (ampGroupCount > 0) {
      //float normalizedFreq = float(averageFreq / sFreqBy2);
      float normalizedFreq = float(maxBinAmpFreq / sFreqBy2);
      normalizedFreq = pow(normalizedFreq, 0.7);
      int targetFreq = int(round(normalizedFreq * 200) + 20);
      targetWaveFrequencies[i] = targetFreq;
      // if (i == 0) {
      //   targetWaveFrequencies[i] = map(maxBinAmpFreq, 0, topOfSample, 20, 220);
      // }
      //targetWaveFrequencies[i] = map(maxBinAmpFreq, 0, int(sFreqBy2), 20, 200);
    }
    // calculate the value to alter the frequency per each update cycle
    if (FADE_RATE > 1) {
      waveFreqStep[i] = float((nextWaveFrequencies[i] - waveFrequencies[i]) / FADE_RATE);
    } else {
      waveFreqStep[i] = float(nextWaveFrequencies[i] - waveFrequencies[i]);
    }

    avgAmpsSum += averageBinsArray[i];

    if (averageBinsArray[i] > maxAvgAmp) {
      maxAvgAmp = averageBinsArray[i];
    }
    lastJ = newJ;
  }
  return avgAmpsSum;
}

/* find all the major peaks in the fft spectrogram, based on the threshold and how many peaks to find */
void findMajorPeaks() {
  int majorPeaksCounter = 0;
  int arraySize = FFT_WIN_SIZE >> 1;
  int peakReached = 0;
  // traverse through spectrogram and find all peaks (above FFT_FLOOR_THRESH)
  for (int i = 1; i < arraySize; i++) { 
    // skip the iteration if the fft amp is an unsually high value to exclude them from calculations (usually occurs on the lowest and highest frequencies)
    if (vReal[i - 1] > OUTLIER) {
      continue;
    }
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
        if(vReal[i - 1] > FFT_FLOOR_THRESH) {
          // print all considered peaks so far
          // Serial.printf("(%d,%d),", i - 1, int(round(vReal[i - 1])));
          peakReached = 1;

          /* array buffering based on the number peak frequencies being returned */
          // if the array is not 'full', iterate peak counter and store peak into array (about 20 lines down)
          if (majorPeaksCounter < numPeaks) {
            majorPeaksCounter++;
          } else {              // otherwise, if array is 'full',
            // set the lowest peak index to the last value in the array
            int lowestPeakIndex = majorPeaksCounter - 1;
            // find the lowest amplitude in the buffer array
            for (int j = 0; j < numPeaks - 1; j++) {
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
            for (int j = lowestPeakIndex; j < numPeaks - 1; j++) {
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
  for (int j = majorPeaksCounter; j < numPeaks; j++) {
    peakAmplitudes[j] = -1;
    peakIndexes[j] = -1;
  }
  // Serial.println();
}


// frequency of max amplitude change
void frequencyMaxAmplitudeDelta() {
  int arrSize = int(FFT_WIN_SIZE) >> 1;
  maxAmpChange = 0;
  int maxAmpChangeI = 0; 
  for (int i = 0; i < arrSize; i++) {
    if (vReal[i] < OUTLIER) {
      int currAmpChange = abs(vReal[i] - vRealPrev[i]);
      if (currAmpChange > maxAmpChange) {
        maxAmpChange = currAmpChange;
        maxAmpChangeI = i;
      }
    }
    vRealPrev[i] = vReal[i];
  }
  peakAmplitudes[numPeaks] = map(vReal[maxAmpChangeI], 0, sumOfPeakAmps, 0, 255);
  
  freqMaxAmpChange = int(round((maxAmpChangeI + 1) * sFreqBySamples));
  targetWaveFrequencies[numPeaks] = map(freqMaxAmpChange, 0, int(sFreqBy2), 16, 200);
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
    //long int sampleTime = micros();
    int i = numSamplesTaken;
    int sample;
    sample = mozziAnalogRead(AUDIO_INPUT_PIN); // Read sample from input
    // store the maximum sample to account for various input voltages
    if (sample > currSampleMax) {
       currSampleMax = sample;
    }
    if (sample < currSampleMin) {
      currSampleMin = sample;
    }
    vReal[i] = (float)sample;                 // For FFT to Bins code
    vImag[i] = 0;                             // set imaginary values to 0
    //while (micros() - sampleTime < sampleDelayTime) {}
    numSamplesTaken++;
    return sample + recordSample(sampleCount - 1);
  }
  else if (numSamplesTaken > FFT_WIN_SIZE - 1) {
    numSamplesTaken = 0;
    if (currSampleMax > sampleMax) {
      sampleMax = currSampleMax;
    }
    sampleRange = currSampleMax - currSampleMin;
    if (sampleRange > sampleRangeMax) {
      sampleRangeMax = sampleRange;
    } else if (sampleRange < sampleRangeMin) {
      sampleRangeMin = sampleRange;
    };
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

void crudeFindFormants()
{
  maleFormantTable[0] = 2160; // i
  maleFormantTable[2] = 1865; // y
  maleFormantTable[3] = 1920; // e
  maleFormantTable[3] = 760; // a
  maleFormantTable[4] = 280; // o
  maleFormantTable[5] = 340; // u

  int maxAmpOne = 0;
  int maxAmpTwo = 0;
  int strongFreqOne = 0;
  int strongFreqTwo = 0;
  //Potentially add a 3rd to account for fundemental freq.

  for(int i = 0; i < FFT_WIN_SIZE / 2; i++)
  {
    Serial.printf("%d\n", vReal[i]);
    if(vReal[i] > 100)
    {
      if(vReal[i] > maxAmpOne)
      {
        maxAmpOne = vReal[i];
        strongFreqOne = i;
        continue;
      }
      else if(vReal[i] > strongFreqTwo)
      {
        maxAmpTwo = vReal[i];
        strongFreqTwo = i;
      }
    }
    
  }

  for(int i = 0; i < 6; i++)
  {
      if(abs(strongFreqOne - strongFreqTwo) > i - i * 0.08 && abs(strongFreqOne - strongFreqTwo) < maleFormantTable[i] + 30)
      {
        //Serial.printf("Formant Found! 1-%d | 2-%d | Difference - %d\n");
      }
      else
      {
        //Serial.printf("Formant not Found 1-%d | 2-%d | Difference - %d\n");
      }
  }


}

void findFormants()
{
  for(int i = 0; i < FFT_WIN_SIZE / 2; i++)
  {
    thisWindow[i] = vReal[i];
  }

  for(int i = 0; i < downsampleCount; i++)
  {

  }
}

void autoCorrelation()
{
  for(int i = 0; i < lpcOrder; i++)
  {
    for(int k = 0; k < FFT_WIN_SIZE / 2; k++)
    {

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