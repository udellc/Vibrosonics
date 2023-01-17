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

#define AUDIO_INPUT_PIN A2
#define RUN_MAX_ARR_SIZE 64  // Array size for the running max of the last x maximum values

#define FREQ_AVG_WIN 6              // Number of FFT windows worth of frequency date to calculate, record, and average when finding the change in dominant frequency of the signal.
#define AMP_AVG_TIME 0.25           // Amount of time to average the signal amplitude across to determine its average amplitude.
#define FREQ_SIG_THRESH 10          // Minimum value required for that the major peak of a signal must meet in order for it to be considered the new dominant frequency of the signal.
#define FREQ_DELTA_ERROR_MARGIN 32  // Minimum difference between dominant frequencies to be considered a significant change.

#define CONTROL_RATE 128

/*/
########################################################
    FFT Setup
########################################################
/*/

arduinoFFT FFT = arduinoFFT(); /* Create FFT object */

/*
These values can be changed in order to evaluate the functions
*/
const uint16_t FFT_WIN_SIZE = 512;  //This value MUST ALWAYS be a power of 2
const float SAMPLE_FREQ = 10000;

/*
These are the input and output vectors
Input vectors receive computed results from FFT
*/
double vReal[FFT_WIN_SIZE];
double vImag[FFT_WIN_SIZE];

/*/
########################################################
    Stuff relevant to splitting fft amplitudes into bins
########################################################
/*/

/*
These are used for splitting the amplitudes from FFT into bins,
  The bins integer accounts for how many bins the samples are split into
  The float curve accounts for the curve used when splitting the bins
*/
int bins = 5;
float curve = 0.5;
const float OUTLIER = 10000.0;

// running max global variables
float runningMax = 0;
float runningMaxArr[RUN_MAX_ARR_SIZE];

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

Oscil<2048, AUDIO_RATE> asinc(SIN2048_DATA);  //asinc is the carrier wave. This is the sum of all other waves and the wave that should be outputted.
Oscil<2048, AUDIO_RATE> asin1(SIN2048_DATA);
Oscil<2048, AUDIO_RATE> asin2(SIN2048_DATA);
Oscil<2048, AUDIO_RATE> asin3(SIN2048_DATA);
Oscil<2048, AUDIO_RATE> asin4(SIN2048_DATA);
Oscil<2048, AUDIO_RATE> asin5(SIN2048_DATA);

const int OscilCount = 6;  //The total number of waves. Modify this if more waves are added, or the program will segfault.

int gainzDivisor;

float sickGainz[OscilCount] = { 0 };  //Set between 0 and 1.
int realGainz[OscilCount] = { 0 };    //Holds translated float value to integer between 0 and 255 for actual gain settings.

long currentCarrier;
long nextCarrier;

int addSynthCarrier;
int addSyntComplete = 0;

/*/
########################################################
    Setup
########################################################
/*/

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < RUN_MAX_ARR_SIZE; i++) {
    runningMaxArr[i] = -1;
  }

  // Additive Synthesis
  startMozzi(CONTROL_RATE);
  asinc.setFreq(16);  //16 Hz - Base shaker sub-harmonic freq.
  asin1.setFreq(30);  //Set frequencies here. (Hardcoded for now)
  asin2.setFreq(40);
  asin3.setFreq(50);
  asin4.setFreq(60);
  asin5.setFreq(70);

  //sickGainz[0] = 0;
  sickGainz[1] = 1;  //Set amplitude here. The array index corresponds to the sin wave number. Use any value between 0-1 for amplitude. (Hardcoded for now)
  sickGainz[2] = 1;
  sickGainz[3] = 1;
  sickGainz[4] = 1;
  sickGainz[5] = 1;
  //Serial.println("endSetup");

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
    }
    delay(1000);
  }

  /*SAMPLING*/
  microseconds = micros();
  for (int i = 0; i < FFT_WIN_SIZE; i++) {
    vReal[i] = analogRead(AUDIO_INPUT_PIN);
    vImag[i] = 0;
    while (micros() - microseconds < sampleDelayTime) {
      //empty loop
    }
    microseconds += sampleDelayTime;
  }

  /*FFT*/
  FFT.Windowing(vReal, FFT_WIN_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD); /* Weigh data */
  FFT.Compute(vReal, vImag, FFT_WIN_SIZE, FFT_FORWARD);                 /* Compute FFT */
  FFT.ComplexToMagnitude(vReal, vImag, FFT_WIN_SIZE);                   /* Compute magnitudes */

  /*FFT to Bins*/
  float averageBinsArray[bins];
  SplitSample(vReal, FFT_WIN_SIZE >> 1, averageBinsArray, bins, curve);
  float normalizedAvgBinsArray[bins];
  normalizeArray(averageBinsArray, normalizedAvgBinsArray, bins);
  //PrintBinsArray(averageBinsArray, bins, curve);
  PrintBinsArray(normalizedAvgBinsArray, bins, curve);

  // Additive Synthesis
  Serial.print('\t');
  sickGainz[1] = normalizedAvgBinsArray[0];
  sickGainz[2] = normalizedAvgBinsArray[1];
  sickGainz[3] = normalizedAvgBinsArray[2];
  sickGainz[4] = normalizedAvgBinsArray[3];
  sickGainz[5] = normalizedAvgBinsArray[4];
  addSyntComplete = 0;
  while (addSyntComplete == 0) {
    audioHook();
  }
  Serial.printf("mVMin:%d\tmVMax:%d\tmV:%d\n", -10, 300, map(addSynthCarrier, -100000, 100000, 0, 255));


  /* for(int i = 0; i < RUN_MAX_ARR_SIZE; i++)  {
    Serial.print(runningMaxArr[i]);
    Serial.print(' ');
  }
  Serial.print('\t');
  Serial.print(runningMax);
  Serial.println(); */

  /*
  // Nanolux Code
  // Calculates the change in dominant frequency of the signal.
  Serial.print(" ");
  recordSamples(freqAvgSampCount);

  calculateFrequencyDelta();

  // Prints the current and previous dominant frequencies of the signal, and the difference between them, to the serial plotter.
  Serial.print("Previous_dom_freq:");
  Serial.print(previousDomFreq);
  Serial.print(", Current_dom_freq:");
  Serial.print(currentDomFreq);
  Serial.print(", Delta:");
  Serial.print(freqDelta);
  Serial.println(); */
  //while (1)
  //  ;
  delay(10); /* Repeat after delay */
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
void SplitSample(double *vData, uint16_t bufferSize, float *destArray, int splitInto, float curveValue) {

  float sFreqBySamples = SAMPLE_FREQ / 2 / bufferSize;
  float step = 1.0 / splitInto;
  float exponent = 1.0 / curveValue;

  float topOfSample = 0;
  int lastJ = 0;
  for (int i = 0; i < splitInto; i++) {
    float xStep = (i + 1) * step;
    // (x/splitInto)^(1/curveValue)
    int binSize = round(bufferSize * pow(xStep, exponent));
    float amplitudeGroup[binSize - lastJ];
    int newJ = lastJ;
    for (int j = lastJ; j < binSize; j++) {
      // if amplitude is above certain threshold, set it to -1
      topOfSample = topOfSample + sFreqBySamples;
      if (vData[j] > OUTLIER || topOfSample < 50) {
        amplitudeGroup[j - lastJ] = -1;
      } else {
        amplitudeGroup[j - lastJ] = vData[j];
      }
      //Debugging stuff
      /*
      Serial.print(j);
      Serial.print(" ");
      Serial.print(vData[j]);
      Serial.print(" ");
      Serial.println(topOfSample, 3); */
      newJ += 1;
    }
    destArray[i] = getArrayMean(amplitudeGroup, binSize - lastJ);
    lastJ = newJ;
  }
}

// gets average of an array
float getArrayMean(float *array, int arraySize) {
  float arraySum = 0.0;
  int c = 0;
  for (int i = 0; i < arraySize; i++) {
    if (array[i] >= 0) {
      arraySum += array[i];
      c += 1;
    }
  }
  if (c == 0) {
    return 0.0;
  }
  return (float)(arraySum / c);
}

// normalizes array and puts the normalized values in destArray
void normalizeArray(float *array, float *destArray, int arraySize) {
  float min = getArrayMin(array, arraySize);
  //float max = getArrayMax(array, arraySize);
  runningMax = getRunningMax(array, arraySize);
  float iv = 1.0 / runningMax;
  for (int i = 0; i < arraySize; i++) {
    float normalizedValue = (array[i]) * iv;
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

float getRunningMax(float *array, int arraySize) {
  // get the maximum in the current bin array of values
  float binArrMax = array[0];
  for (int i = 1; i < arraySize; i++) {
    if (array[i] > binArrMax) {
      binArrMax = array[i];
    }
  }
  // look through the last RUN_MAX_ARR_SIZE number of maximums and return the max out of those values
  int i = 0;
  for (i; i < RUN_MAX_ARR_SIZE; i++) {
    // if the runningMaxArr[i] is empty then store the bins array max
    if (runningMaxArr[i] == -1) {
      runningMaxArr[i] = binArrMax;
      break;
    }
  }
  // find the maximum in the running max array
  float lastMax = getArrayMax(runningMaxArr, RUN_MAX_ARR_SIZE);
  // if iterator was at last location, make the first location in array empty
  if (i == RUN_MAX_ARR_SIZE - 1) {
    runningMaxArr[0] = -1;
  } else {  // otherwise, make the next location empty
    runningMaxArr[i + 1] = -1;
  }
  return lastMax;
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
    sprintf(buffer, "%.2f for frequency range between %d and %d Hz\n", array[i], rangeLow, rangeHigh);
    Serial.print(buffer);
  }
}

/*/
########################################################
    Additive Synthesis Functions
########################################################
/*/

void sickGainzTranslation(float gainz[])  //Used to translate the 0-255 amplitude scale to a 0-1 float scale.
{
  for (int i = 0; i < OscilCount; i++) {
    if (gainz[i] > 1 || gainz[i] < 0) {
      Serial.println("ERROR: Gain value out of bounds");
    } else if (gainz[i] > 0) {
      realGainz[i] = (int)(gainz[i] * 255);
    } else {
      realGainz[i] = 0;
    }
  }
}


void updateControl() {
  //Serial.println("updateControlStart");
  sickGainzTranslation(sickGainz);
  //Serial.println("after translation");
  gainzControl();
  //Serial.println("after control");

  //========================================== DEBUG BELOW
  // for(int i = 0; i < OscilCount; i++)
  //{
  //  Serial.printf("Gain %d: %f\n", i, sickGainz[i]);
  // }

  //Serial.printf("currentCarrier: %ld | nextCarrier: %ld\n", currentCarrier, nextCarrier);
  addSynthCarrier = currentCarrier;
  addSyntComplete = 1;
  //Serial.println("updateControlend");
  sickGainz[0] = 0;
  sickGainz[1] = 0;
}

void gainzControl()  //Used for amplitude modulation so it does not exceed the 255 cap.
{
  int totalGainz = 0;
  gainzDivisor = 0;

  for (int i = 0; i < OscilCount; i++) {
    totalGainz += realGainz[i];
  }

  while (totalGainz > 255 && totalGainz != 0) {
    //Serial.println(totalGainz);
    totalGainz >>= 1;
    gainzDivisor++;
  }
}

int updateAudio() {
  //Serial.println("updateAudioStart");
  currentCarrier = ((long)asinc.next() + realGainz[1] * asin1.next() + realGainz[2] * asin2.next() + realGainz[3] * asin3.next() + realGainz[4] * asin4.next() + realGainz[5] * asin5.next());  //Additive synthesis equation


  nextCarrier = currentCarrier >> gainzDivisor;
  //Serial.println("updateAudioEnd");
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
