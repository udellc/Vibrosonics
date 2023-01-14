#include <Arduino.h>
#include "arduinoFFT.h"
#include "esp32FFTtoParabolicBins.h"

#define CHANNEL A2

arduinoFFT FFT = arduinoFFT(); /* Create FFT object */
/*
These values can be changed in order to evaluate the functions
*/
const uint16_t samples = 512;  //This value MUST ALWAYS be a power of 2
const double samplingFrequency = 8192;

const double OUTLIER = 5000.0;
/*
These are the input and output vectors
Input vectors receive computed results from FFT
*/
double vReal[samples];
double vImag[samples];

double vRealNormalized[samples];


unsigned int sampling_period_us;
unsigned long microseconds;

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

void FFTtoParaSetup() {
  sampling_period_us = round(1000000 * (1.0 / samplingFrequency));
  //Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Ready");
}

void FFTtoParaLoop() {

  /*SAMPLING*/
  microseconds = micros();
  for (int i = 0; i < samples; i++) {
    vReal[i] = analogRead(CHANNEL);
    vImag[i] = 0;
    while (micros() - microseconds < sampling_period_us) {
      //empty loop
    }
    microseconds += sampling_period_us;
  }

  FFT.Windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD); /* Weigh data */
  FFT.Compute(vReal, vImag, samples, FFT_FORWARD);                 /* Compute FFT */
  FFT.ComplexToMagnitude(vReal, vImag, samples);                   /* Compute magnitudes */
  //PrintVector(vReal, samples, SCL_FREQUENCY);
  SplitSample(vReal, samples, 8, 0.3);
  // Serial.println(x);
  // while (1)
  //   ; /* Run Once
  delay(10); /* Repeat after delay */
}

void PrintVector(double *vData, uint16_t bufferSize, uint8_t scaleType) {
  for (uint16_t i = 0; i < bufferSize; i++) {
    double abscissa;
    /* Print abscissa value */
    switch (scaleType) {
      case SCL_INDEX:
        abscissa = (i * 1.0);
        break;
      case SCL_TIME:
        abscissa = ((i * 1.0) / samplingFrequency);
        break;
      case SCL_FREQUENCY:
        abscissa = ((i * 1.0 * samplingFrequency) / samples);
        break;
    }
    if (scaleType != SCL_PLOT) {
      Serial.print(abscissa, 6);
      if (scaleType == SCL_FREQUENCY)
        Serial.print("Hz");
      Serial.print(" ");
    }
    Serial.println(vData[i], 4);
  }
  Serial.println();
}

/* Splits the bufferSize into X groups, where X = splitInto
    the curveValue determines the curve to follow when buffer is split
    if curveValue = 1 : then buffer is split into X even groups
    0 < curveValue < 1 : then buffer is split following a concave curve
    1 < curveValue < infinity : then buffer is split following a convex curve */
void SplitSample(double *vData, uint16_t bufferSize, int splitInto, double curveValue) {
  double splitMeanArray[splitInto];

  int sFreqBySamples = samplingFrequency / samples;
  double step = 1.0 / splitInto;
  double exponent = 1 / curveValue;

  int topOfSample = 0;
  int lastJ = 0;
  for (int i = 0; i < splitInto; i++) {
    double xStep = (i + 1) * step;
    // (x/splitInto)^(1/curveValue)
    int binSize = round(bufferSize * pow(xStep, exponent));
    double amplitudeGroup[binSize - lastJ];
    int newJ = lastJ;
    // Serial.println(binSize - lastJ);
    for (int j = lastJ; j < binSize; j++) {
      // if amplitude is above certain threshold, set it to -1
      topOfSample = topOfSample + sFreqBySamples;
      if (vData[j] > OUTLIER || (topOfSample < 64 || topOfSample > 8128)) {
        amplitudeGroup[j - lastJ] = -1;
      } else {
        amplitudeGroup[j - lastJ] = vData[j];
      }
      // Debugging stuff
      // Serial.print(j);
      // Serial.print(" ");
      // Serial.print(vData[j]);
      // Serial.print(" ");
      // Serial.println(topOfSample);
      newJ += 1;
    }
    splitMeanArray[i] = getArrayMean(amplitudeGroup, binSize - lastJ);
    lastJ = newJ;
  }

  //double normalizedArray[splitInto];

  //normalizeArray(splitMeanArray, normalizedArray, splitInto);

  PrintArray(splitMeanArray, splitInto, curveValue);
  //PrintArray(normalizedArray, splitInto, curveValue);
}

// gets average of an array
double getArrayMean(double *array, int arraySize) {
  double arraySum = 0.0;
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
  return (double)(arraySum / c);
}

// normalizes array and puts the normalized values in destArray
void normalizeArray(double *array, double *destArray, int arraySize) {
  double min = getArrayMin(array, arraySize);
  double max = getArrayMax(array, arraySize);

  double iv = 1.0 / max;
  for (int i = 0; i < arraySize; i++) {
    double normalizedValue = (array[i]) * iv;
    destArray[i] = normalizedValue;
  }
}

// returns the minimum value in array
double getArrayMin(double *array, int arraySize) {
  double min = array[0];
  for (int i = 1; i < arraySize; i++) {
    if (array[i] < min) {
      min = array[i];
    }
  }
  return min;
}

// returns the maximum value in array
double getArrayMax(double *array, int arraySize) {
  double max = array[0];
  for (int i = 1; i < arraySize; i++) {
    if (array[i] > max) {
      max = array[i];
    }
  }
  return max;
}


// prints average amplitudes array
void PrintArray(double *array, int arraySize, double curveValue) {
  Serial.println("\nPrinting amplitudes:");
  int baseMultiplier = samplingFrequency / samples;

  double step = 1.0 / arraySize;
  double parabolicCurve = 1 / curveValue;

  for (int i = 0; i < arraySize; i++) {
    double xStep = i * step;
    char buffer[64];
    // char normalizedAmplitude = char(array[i]);
    int rangeLow = 0;
    if (i > 0) {
      rangeLow = round(samplingFrequency * pow(xStep, parabolicCurve));     
    }
    int rangeHigh = round(samplingFrequency * pow(xStep + step, parabolicCurve));   
    sprintf(buffer, "%.3f for frequency range between %d and %d Hz\n", array[i], rangeLow, rangeHigh);
    Serial.print(buffer);
  }
}