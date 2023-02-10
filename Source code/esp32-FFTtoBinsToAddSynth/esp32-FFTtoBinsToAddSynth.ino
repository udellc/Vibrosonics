#include <arduinoFFT.h>
#include <Arduino.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>
#include <string>

/*/
########################################################
    Directives
########################################################
/*/

#define AUDIO_INPUT_PIN A2          // audio in pin

#define RUN_MAX_ARR_SIZE 16         // Array size for the running max of the last x maximum values
#define RAW_INPUT_ARR_SIZE 16       // Array size for the running raw audio input, for calculating standard deviation

#define INPUT_STD_DEVIATION_THRESH 5.0 // The minimum standard deviation threshold of the running raw audio input samples required to shake basshaker

#define FREQ_AVG_WIN 6              // Number of FFT windows worth of frequency date to calculate, record, and average when finding the change in dominant frequency of the signal.
#define AMP_AVG_TIME 0.25           // Amount of time to average the signal amplitude across to determine its average amplitude.
#define FREQ_SIG_THRESH 10          // Minimum value required for that the major peak of a signal must meet in order for it to be considered the new dominant frequency of the signal.
#define FREQ_DELTA_ERROR_MARGIN 32  // Minimum difference between dominant frequencies to be considered a significant change.

#define CONTROL_RATE 128

#define REF_VOLTAGE 0.805861 // 12 bit ADC to voltage conversion 3300/4095

/*/
########################################################
    FFT Setup
########################################################
/*/

/*
These values can be changed in order to evaluate the functions
*/
const uint16_t FFT_WIN_SIZE = 512;  //This value MUST ALWAYS be a power of 2
const float SAMPLE_FREQ = 10000;    //The sampling frequency

/*
These are the input and output vectors
  - vReal is used for input and receives computed results from FFT
  - vImag is used to store imaginary values for computation
*/
float vReal[FFT_WIN_SIZE];
float vImag[FFT_WIN_SIZE];

arduinoFFT FFT = arduinoFFT(vReal, vImag, FFT_WIN_SIZE, SAMPLE_FREQ); // Object for performing FFT's

/*/
########################################################
    Stuff relevant to splitting fft amplitudes into bins
########################################################
/*/

/*
These are used for splitting the amplitudes from FFT into bins,
  - bins integer accounts for how many bins the samples are split into
  - float curve accounts for the curve used when splitting the bins
  - outlier is used to remove unusually high amplitudes from FFT analysis
*/
int bins = 5;
float curve = 0.5;
const float OUTLIER = 25000.0;

/* running max and raw input array
  - runningMaxArr stores the last RUN_MAX_ARR_SIZE maximum amplitudes
  - rawInputArr stores the last RAW_INPUT_ARR_SIZE raw audio input for calculating standard deviation
*/
int sampleMax = 1;
float inputStandardDeviationN = 0.0;
float inputStandardDeviationMax = 0.0;
float runningMaxArr[RUN_MAX_ARR_SIZE];
float rawInputArr[RAW_INPUT_ARR_SIZE];

unsigned long microseconds;

/*/
########################################################
    Stuff relevant to all of the signal processing functions (calculateCentroid, calculateFrequencyDelta, and calculateAvgAmplitude)
########################################################
/*/
// Number of microseconds to wait between recording samples.
const int sampleDelayTime = round(1000000 / SAMPLE_FREQ);

// Number of samples that should be averaged to calculate the average amplitude of the signal.
const int ampAvgSampCount = round(AMP_AVG_TIME * SAMPLE_FREQ);

// Number of samples required to calculate the all of the FFT windows that are averaged when finding changes in the dominant frequency of the signal.
const int freqAvgSampCount = FREQ_AVG_WIN * FFT_WIN_SIZE;

// Size of the sample buffer array. This expression is equivalent to max(ampAvgSampCount, freqAvgSampCount), but I couldn't use max() because of a weird error.
const int sampleBuffSize = ampAvgSampCount * (ampAvgSampCount > freqAvgSampCount) + freqAvgSampCount * (ampAvgSampCount <= freqAvgSampCount);

// Array that samples are first recorded into. Its size is equal to the minimum size required for performing all of the signal processing.
float samples[sampleBuffSize];

// The number of audio samples taken, used to synchronously implement Nanolux code with the current FFT implementation
int sampleBuffCount = 0;

/*/
########################################################
    Stuff relevant to calculateFreqDelta
########################################################
/*/
// Array used for storing frequency magnitudes from multiple FFT's of the signal. The first index indicates the specific FFT window (and can be thought of as the time at
// which each FFT window was calculated), and the second index indicates the sample index within each frequency window (and can be though of as frequency).
float freqs[FREQ_AVG_WIN][FFT_WIN_SIZE / 2];

// Previously recorded dominant frequency of the signal, calculated by calculateFreqDelta.
float previousDomFreq = 0;

// Current dominant frequency of the signal, calculated by calculateFreqDelta.
float currentDomFreq = 0;

// Difference between the previous and the current dominant frequency of the signal, calculated by calculateFreqDelta.
float freqDelta = 0;

/*/
########################################################
    Stuff relavent to additive synthesizer
########################################################
/*/

Oscil <2048, AUDIO_RATE> asinc(SIN2048_DATA); //asinc is the carrier wave 20Hz. This is the sum of all other waves and the wave that should be outputted.
Oscil <2048, AUDIO_RATE> asin1(SIN2048_DATA); //20Hz
Oscil <2048, AUDIO_RATE> asin2(SIN2048_DATA); //30 - 39
Oscil <2048, AUDIO_RATE> asin3(SIN2048_DATA); //40 - 49
Oscil <2048, AUDIO_RATE> asin4(SIN2048_DATA); //50 - 59
//Oscil <2048, AUDIO_RATE> asin5(SIN2048_DATA); //60 - 69 // These are added for later, when changing the number of bins
//Oscil <2048, AUDIO_RATE> asin6(SIN2048_DATA); //
//Oscil <2048, AUDIO_RATE> asin6(SIN2048_DATA); //
//Oscil <2048, AUDIO_RATE> asin6(SIN2048_DATA); //
//Oscil <2048, AUDIO_RATE> asin6(SIN2048_DATA); //
//Oscil <2048, AUDIO_RATE> asin6(SIN2048_DATA); //
//Oscil <2048, AUDIO_RATE> asin6(SIN2048_DATA); //
//Oscil <2048, AUDIO_RATE> asin6(SIN2048_DATA); //
//Oscil <2048, AUDIO_RATE> asin6(SIN2048_DATA); //
//Oscil <2048, AUDIO_RATE> asin6(SIN2048_DATA); //
//Oscil <2048, AUDIO_RATE> asin6(SIN2048_DATA); //
//Oscil <2048, AUDIO_RATE> asin6(SIN2048_DATA); //80Hz
Oscil <2048, AUDIO_RATE> asind(SIN2048_DATA); //15-75Hz, full range for Nanolux code

const int OscilCount = 6;  //The total number of waves. Modify this if more waves are added, or the program will segfault.

float amplitudeGains[OscilCount] = { 0.0 };  //Set between 0 and 1.

long currentCarrier;
long nextCarrier;

// nanolux domFreq
int domFreq = 0;

// this variable changes to 1 when additive synthesis is complete for 'seamless' operation
int addSyntComplete = 0;

/*/
########################################################
  Other
########################################################
/*/

int opmode = 0;


/*/
########################################################
    Setup
########################################################
/*/

void setup() {
  // set baud rate
  Serial.begin(115200);
  
  // setup pins
  pinMode(AUDIO_INPUT_PIN, INPUT);
  analogReadResolution(12);

  // setup running max array
  for (int i = 0; i < RUN_MAX_ARR_SIZE; i++) {
    runningMaxArr[i] = -1;
  }

  // setup raw input array
  for (int i = 0; i < RAW_INPUT_ARR_SIZE; i++) {
    rawInputArr[i] = -1;
  }

  // Additive Synthesis
  startMozzi(CONTROL_RATE);
  asinc.setFreq(16);  //16 Hz - Base shaker sub-harmonic freq.
  asin1.setFreq(20);  //Set frequencies here. (Hardcoded for now)
  asin2.setFreq(30);
  asin3.setFreq(40);
  asin4.setFreq(50);
  //asin5.setFreq(70);
  asind.setFreq(0);  //Nanolux dominant frequency value changes when enough samples are taken

  amplitudeGains[5] = 0.0;

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

  /* Output string */
  char buffer[400];
  memset(buffer, 0, sizeof(buffer));
  strcat(buffer, "Amps: ");

  /* set number of bins and curve value */
  while (Serial.available()) {
    char data = Serial.read();
    if (data == 'x') {
      Serial.println("Enter integer for number of bins:");
      while (Serial.available() == 1) {}
      bins = Serial.parseInt();
      Serial.println("Enter float for curve value:");
      while (Serial.available() == 1) {}
      curve = Serial.parseFloat();
      Serial.println("New bins and curve value set.");
      Serial.printf("bins: %d\tcurve: %.2f\n", bins, curve);
    }
    delay(500);
  }

  /* SAMPLING */
  int sample;
  int sampleSum = 0;
  float inputStandardDeviation = 0.0;
  for (int i = 0; i < FFT_WIN_SIZE; i++) {
    sample = analogRead(AUDIO_INPUT_PIN); // Read sample from input
    sampleSum += sample;
    // store the maximum sample to account for various input voltages
    if (sample > sampleMax) {
       sampleMax = sample;
    }
    vReal[i] = (float)sample;             // For FFT to Bins code
    samples[sampleBuffCount] = (float)sample; // For Nanolux implementation
    sampleBuffCount++;
    vImag[i] = 0;                         // set imaginary values to 0
    //Serial.printf("Min_mV:%d\tMax_mV:%d\tSample_mV:%d\n", 0, 255, sample); // print raw audio input
    while (micros() - microseconds < sampleDelayTime) {
      //empty loop
    }
  }
  float sampleAvg = float(sampleSum /= FFT_WIN_SIZE);
  // Get the the standard deviation of the rawInputArr which contains the last RUN_INPUT_ARR_SIZE samples from each sampling session
  inputStandardDeviation = getRunningStandardDeviation(sampleAvg);
  if (inputStandardDeviation > inputStandardDeviationMax && inputStandardDeviationMax + 100 > INPUT_STD_DEVIATION_THRESH) {
    inputStandardDeviationMax = inputStandardDeviation;
  } else {
    inputStandardDeviationMax * 0.99;
  }
  // put standard deviation value in 0-1.0 range
  inputStandardDeviationN = inputStandardDeviation / inputStandardDeviationMax;

  /* FFT */
  FFT.DCRemoval();                                  // Remove DC component of signal
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
  FFT.Compute(FFT_FORWARD);                         // Compute FFT
  FFT.ComplexToMagnitude();                         // Compute frequency magnitudes

  /* FFT to Bins */
  float averageBinsArray[bins];
  SplitSample(vReal, FFT_WIN_SIZE >> 1, averageBinsArray, bins, curve);
  float normalizedAvgBinsArray[bins];
  normalizeArray(averageBinsArray, normalizedAvgBinsArray, bins);
  // form output string
  formBinsArrayString(normalizedAvgBinsArray, bins, buffer, curve);

  float totalGains = 0.0;
  // mapping each average normalized bin from FFT analysis to a sine wave gain value for additive synthesis
  for (int i = 0; i < bins; i++) {
    // if the AUDIO_INPUT is relatively unchanged, the gain value is set to 0
    if (inputStandardDeviation < INPUT_STD_DEVIATION_THRESH) {
      amplitudeGains[i] = 0.0;
      //amplitudeGains[5] = 0.0;
    } else { // Otherwise the normalized average ampltitude is assigned to the gain value
      //normalizedAvgBinsArray[i] = normalizedAvgBinsArray[i] * 
      float gainValue = normalizedAvgBinsArray[i]; // pow(inputStandardDeviationN, 1.25);
      amplitudeGains[i] = gainValue;
      totalGains += gainValue;
    }
  }

  /* Nanolux Code Implementation */
  if (sampleBuffCount == freqAvgSampCount) {    // run nanolux freqDelta when the samples buffer is full
    sampleBuffCount = 0;
    calculateFrequencyDelta();

    // Prints the current and previous dominant frequencies of the signal, and the difference between them, to the serial plotter.
    // Serial.print("Previous_dom_freq:");
    // Serial.print(previousDomFreq);
    // Serial.print(", Current_dom_freq:");
    // Serial.print(currentDomFreq);
    // Serial.print(", Delta:");
    // Serial.print(freqDelta);
    // Serial.println();

    // Map the dominant frequency to a basshaker frequency
    if (currentDomFreq > 3000) {
       domFreq = 75;
    }
    else {
     domFreq = map(currentDomFreq, 0, 3000, 15, 75);    
    }
    asind.setFreq(domFreq); // set the mapped frequency to associated wave
  }
  // set the amplitude for the wave, uses the rest of the available range (between 0.0 and 1.0)
  float domFreqAmp = 0.25; //* pow(inputStandardDeviationN, 2);
  //amplitudeGains[5] = domFreqAmp;
  
  // form output string
  char buffer2[40];
  sprintf(buffer2, "NDF Gain: %.2f\tInputStdDev: %.2f\t", domFreqAmp, inputStandardDeviationN);
  strcat(buffer, buffer2);

  /* Additive Synthesis */

  addSyntComplete = 0;
  // wait for additive synthesis to generate a carrier wave
  while (addSyntComplete == 0) {
    audioHook();
  }
  
  /* Serial Ouput */
  // print carrier wave, mapped to 0-255
  char buffer3[128];
  sprintf(buffer3, "Plotter: Min:%d\tMax:%d\tOutput(0-255):%d\tInput(0-255):%d\tDomFreq(15-75Hz):%d\n", 0, 255, map((int)nextCarrier, -127, 127, 0, 255), map(sample, 0, sampleMax, 0, 255), domFreq);
  strcat(buffer, buffer3);
  Serial.print(buffer);

  // Loop after reading in character 'n' for debugging
  /*
  char dataT = '0';
  while (Serial.available() == 0) {
    char dataT = Serial.read();
    if (dataT == 'n') {
      continue;
    }
  }
  */
}

/*/
########################################################
    FFT to Bins Functions
########################################################
/*/

/* Splits the bufferSize into X groups, where X = splitInto
    the curveValue determines the curve to follow when buffer is split
    if curveValue = 1 : then buffer is split into X even groups
    0 < curveValue < 1 : then buffer is split into X groups, following a concave curve
    1 < curveValue < infinity : then buffer is split into X groups following a convex curve */
void SplitSample(float *vData, uint16_t bufferSize, float *destArray, int splitInto, float curveValue) {

  float sFreqBySamples = SAMPLE_FREQ / 2 / bufferSize;  // determine what frequency each amplitude represents. NOTE: unsure if it is accurate
  // (x/splitInto)^(1/curveValue)
  float step = 1.0 / splitInto;                         // how often to step on the x-axis to determine bin value
  float exponent = 1.0 / curveValue;                    // power of the parabolic curve (x^exponent)

  float topOfSample = 0;                                // The frequency of the current amplitude
  int lastJ = 0;                                        // the last amplitude taken from vReal
  for (int i = 0; i < splitInto; i++) {                 // Calculate the size of each bin and the amplitudes into each bin
    float xStep = (i + 1) * step;                       // x-axis step
    // (x/splitInto)^(1/curveValue)
    int binSize = round(bufferSize * pow(xStep, exponent));
    float amplitudeGroup[binSize - lastJ];              // array for storing amplitudes from vReal
    int newJ = lastJ;
    for (int j = lastJ; j < binSize; j++) {             // for the next group of amplitudes
      topOfSample = topOfSample + sFreqBySamples;       // calculate the associated frequency
      // if amplitude is above certain threshold, set it to -1 to exclude from signal processing
      if (vData[j] > OUTLIER || topOfSample < 80) {
        amplitudeGroup[j - lastJ] = -1;
      } else {
        amplitudeGroup[j - lastJ] = vData[j];
      }
      // Debugging stuff
      // Serial.printf("I: %d\tFreq: %.2f\tAmp: %.2f\t", j, topOfSample, vData[j]);
      // Serial.println();
      newJ += 1;
    }
    destArray[i] = getArrayMean(amplitudeGroup, binSize - lastJ);
    lastJ = newJ;
  }
}

// gets average of an array
float getArrayMean(float *array, int arraySize) {
  float arraySum = 0.0;
  int n = 0;      // the number of valid values
  for (int i = 0; i < arraySize; i++) {
    // if the value in array is valid, use it to calculate the mean
    if (array[i] >= 0) {
      arraySum += array[i];
      n += 1;     // increment count
    }
  }
  if (n == 0) {   // if all values are invalid, return 0.0 as average
    return 0.0;
  }
  return (float)(arraySum / n); // otherwise return the average
}

// normalizes array and puts the normalized values in destArray
void normalizeArray(float *array, float *destArray, int arraySize) {
  float min = getArrayMin(array, arraySize);
  //float max = getArrayMax(array, arraySize);
  float sum = 0;
  for (int i = 0; i < arraySize; i++) {
    sum += array[i];
  }
  sum = sum + sum * 0.25; // leave 0.25 for ndf
  float iv = 0;     // initialization vector
  if (sum > 0) {
    iv = 1.0 / sum;
  }
  for (int i = 0; i < arraySize; i++) {
    float normalizedValue = array[i] * iv;
    destArray[i] = normalizedValue;
  }
}

// returns the minimum value in array
float getArrayMin(float *array, int arraySize) {
  float min = array[0];
  for (int i = 1; i < arraySize; i++) {
    if (array[i] < min) {
      min = array[i];
    }
  }
  return min;
}

// returns the maximum value in array
float getArrayMax(float *array, int arraySize) {
  float max = array[0];
  for (int i = 1; i < arraySize; i++) {
    if (array[i] > max) {
      max = array[i];
    }
  }
  return max;
}

// store the last max and get the running max of the running max array
float getRunningMax(float lastSampleMax) {
  // look through the last RUN_MAX_ARR_SIZE number of maximums and return the max out of those values
  int i = 0;
  for (i; i < RUN_MAX_ARR_SIZE; i++) {
    // if the runningMaxArr[i] is empty then store the bins array max
    if (runningMaxArr[i] == -1) {
      runningMaxArr[i] = lastSampleMax;
      break;
    }
  }
  float max = getArrayMax(runningMaxArr, RUN_MAX_ARR_SIZE);
  // if iterator was at last location, make the first location in array empty
  if (i == RUN_MAX_ARR_SIZE - 1) {
    // find the maximum in the running max array
    runningMaxArr[0] = -1;
  } else {  // otherwise, make the next location empty
    runningMaxArr[i + 1] = -1;
  }
  return max;
}

// Get the running standard deviation of the running raw input array
float getRunningStandardDeviation(float lastSample) {
  int i = 0;
  for (i; i < RAW_INPUT_ARR_SIZE; i++) {
    // if position in rawInputArr[i] is empty then store the passed value
    if (rawInputArr[i] == -1) {
      rawInputArr[i] = lastSample;
      break;
    }
  }
  // get the standard deviation each time a new value is stored (changed to each time buffer is full for better performance)
  float standardDeviation = calculateStandardDeviation(rawInputArr, RAW_INPUT_ARR_SIZE);
  //float standardDeviation = 0.0;

  // if iterator was at last location, make the first location in array empty
  if (i == RAW_INPUT_ARR_SIZE - 1) {
    //standardDeviation = calculateStandardDeviation(rawInputArr, RAW_INPUT_ARR_SIZE);
    rawInputArr[0] = -1;    // make first location in buffer empty
  } else {  // otherwise, make the next location empty
    rawInputArr[i + 1] = -1;
  }
  return standardDeviation;
}

// calculate standard deviation : s = sqrt(sum((arr[i] - arr_mean)^2) / size)
float calculateStandardDeviation(float *array, int arraySize) {
  float average = getArrayMean(array, arraySize);
  float variance = 0;
  int n = 0;    // the number of valid values
  for(int i = 0; i < arraySize; i++) {
    if (array[i] >= 0) {
      variance += pow(array[i] - average, 2);
      n += 1;   // increment n
    }
  }
  if (n == 0) {
    return 0.0;
  }
  return (float)sqrt(variance / n);
}

// prints average amplitudes array
void PrintBinsArray(float *array, int arraySize, float curveValue) {
  Serial.println("\nPrinting amplitudes:");
  int freqHigh = SAMPLE_FREQ / 2;
  int baseMultiplier = freqHigh / FFT_WIN_SIZE;

  float step = 1.0 / arraySize;
  float parabolicCurve = 1 / curveValue;

  for (int i = 0; i < arraySize; i++) {
    float xStep = i * step;
    char buffer[64];
    int rangeLow = 0;
    if (i > 0) {
      rangeLow = round(freqHigh * pow(xStep, parabolicCurve));
    }
    int rangeHigh = round(freqHigh * pow(xStep + step, parabolicCurve));
    sprintf(buffer, "Avg. amp for frequency range between %d and %dHz: %.2f\n", rangeLow, rangeHigh, array[i]);
    Serial.print(buffer);
  }
}

// form bins array string
void formBinsArrayString(float *array, int arraySize, char *destBuffer, float curveValue) {
  int freqHigh = SAMPLE_FREQ / 2;
  int baseMultiplier = freqHigh / FFT_WIN_SIZE;

  float step = 1.0 / arraySize;
  float parabolicCurve = 1 / curveValue;

  for (int i = 0; i < arraySize; i++) {
    float xStep = i * step;
    char buffer[24];
    memset(buffer, 0, sizeof(buffer));
    int rangeLow = 0;
    if (i > 0) {
      rangeLow = round(freqHigh * pow(xStep, parabolicCurve));
    }
    int rangeHigh = round(freqHigh * pow(xStep + step, parabolicCurve));
    sprintf(buffer, "%d - %dHz: %.2f\t", rangeLow, rangeHigh, array[i]);
    strcat(destBuffer, buffer);
  }
}

/*/
########################################################
    Additive Synthesis Functions
########################################################
/*/

void updateControl() {
  //Serial.println("updateControlStart");

  //========================================== DEBUG BELOW
  // for(int i = 0; i < OscilCount; i++)
  // {
  //   Serial.printf("Gain %d: %f\n", i, amplitudeGains[i]);
  // }

  //Serial.printf("currentCarrier:%ld | nextCarrier:%ld\n", currentCarrier, nextCarrier);
  addSyntComplete = 1;
  //Serial.println("updateControlend");
}

int updateAudio() {
  currentCarrier = (long)(amplitudeGains[0] * asinc.next() + amplitudeGains[1] * asin1.next() + amplitudeGains[2] * asin2.next() + amplitudeGains[3] * asin3.next() + amplitudeGains[4] * asin4.next());  //Additive synthesis equation

  //currentCarrier *= 0.75;

  currentCarrier += (long)(amplitudeGains[5] * asind.next());
  // for(int i = 0; i < OscilCount; i++)
  // {
  //     currentCarrier += realGainz[i] * asinArr[i].next();
  // }
  nextCarrier = (int)(currentCarrier * inputStandardDeviationN);
  return (int)nextCarrier;
}

/*/
########################################################
    Nanolux 
########################################################
/*/

// Calculates the change in dominant frequency of the signal, and stores the new result in currentDomFreq. The previous value is moved to previousDomFreq, and the
// difference between these values is stored in freqDelta. The function works by calculating the magnitudes of the FFT's of several sequential parts of the signal
// and averaging them, and then finding the frequency and value associated with the highest peak in this average. The value of the new peak must have a minimum
// magnitude (the threshhold for which is defined by []), and its frequency must be different from the previously recorded dominant frequency by a minimum amount
// (which is defined by []) in order for the new values of currentDomFreq, previousDomFreq, and freqDelta to be calculated.
void calculateFrequencyDelta() {
  // Calculates the magnitudes of the FFT's of each sequential segment of the signal.
  // Loops over each FFT window
  for (int i = 0; i < FREQ_AVG_WIN; i++) {
    // Loops over vReal, and sets each value equal to a sample from the sample buffer.
    // Also zeroes out vImag.
    for (int s = 0; s < FFT_WIN_SIZE; s++) {
      vReal[s] = samples[i * FFT_WIN_SIZE + s];
      vImag[s] = 0;
    }

    // Calculates magnitudes of the FFT of this segment of the signal.
    FFT.DCRemoval();                                  // Remove DC component of signal
    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
    FFT.Compute(FFT_FORWARD);                         // Compute FFT
    FFT.ComplexToMagnitude();                         // Compute frequency magnitudes

    // Moves the resulting frequency magnitudes in vReal into freqs[][], so that it can be averaged with the other windows later.
    for (int s = 0; s < FFT_WIN_SIZE / 2; s++) {
      freqs[i][s] = vReal[s];
    }
  }

  // Averages all of the FFT windows together across time, to get the average frequency composition of the signal over the past short period.
  // Adds the magnitudes of the frequencies stored in every frequency window onto the first first frequency window (freqs[0][...]), which results in the first
  // frequency window containing the sum of all of them.
  for (int i = 1; i < FREQ_AVG_WIN; i++) {
    for (int f = 0; f < FFT_WIN_SIZE / 2; f++) {
      freqs[0][f] += freqs[i][f];
    }
  }

  // Divides each value in the first frequency window by the number of frequency windows that are being averaged across, to yield the average frequency composition
  // of the signal.
  for (int i = 0; i < FFT_WIN_SIZE / 2; i++) {
    freqs[0][i] /= FREQ_AVG_WIN;
  }

  // Creates variables for temporarily storing the magnitude and frequency of the current dominant frequency of the signal.
  int majorPeakIdx = 0;
  float majorPeakVal = 0;

  // Loops over the first sample window, and finds the x and y values of its maximum. Skips the first value (i=0), because this corresponds to the DC component of
  // the signal (0 Hz), which we don't care about.
  for (int i = 1; i < FFT_WIN_SIZE / 2; i++) {
    // If the value of the peak at this location exceeds the value of the previously recorded maximum peak, store the value of this one instead.
    if (freqs[0][i] > majorPeakVal) {
      majorPeakVal = freqs[0][i];
      majorPeakIdx = i;
    }
  }

  // Checks if the newly recorded major peak exceeds the required significance threshhold, and if it varies from the previous dominant frequency by a meaningful amount.
  // If it meets both of these requirements, record the value into currentDomFreq as the new dominant frequency of the signal.
  if ((majorPeakVal > FREQ_SIG_THRESH) && (abs((majorPeakIdx * (SAMPLE_FREQ / FFT_WIN_SIZE)) - currentDomFreq) > FREQ_DELTA_ERROR_MARGIN)) {
    previousDomFreq = currentDomFreq;                              // Move the previously dominant frequency of the signal into previousDomFreq
    currentDomFreq = majorPeakIdx * (SAMPLE_FREQ / FFT_WIN_SIZE);  // Calculate and store the current dominant frequency of the signal in currentDomFreq
    freqDelta = currentDomFreq - previousDomFreq;                  // Calculate the difference between the current and previous dominant frequencies of the signal.
  }
}

// Records [count] samples into the array samples[] from the audio input pin, at a sampling frequency of SAMPLE_FREQ
void recordSamples(int count) {
  // Records [count] samples.
  for (int i = 0; i < count; i++) {
    // Gets the current time, in microseconds.
    long int sampleTime = micros();

    // Records a sample, and stores it in the current index of samples[]
    samples[i] = analogRead(AUDIO_INPUT_PIN);

    // Wait until the next sample is ready to be recorded
    while (micros() - sampleTime < sampleDelayTime) {}
  }
}
