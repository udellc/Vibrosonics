#include <arduino.h>
#include "arduinoFFT.h"

#define CHANNEL A2

arduinoFFT FFT = arduinoFFT(); /* Create FFT object */
/*
These values can be changed in order to evaluate the functions
*/
const uint16_t samples = 512;  //This value MUST ALWAYS be a power of 2
const double signalFrequency = 1000;
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

void setup() {
  sampling_period_us = round(1000000 * (1.0 / samplingFrequency));
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Ready");
}

void loop() {

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
  SplitSample(vReal, samples, 8);
  // double x = FFT.MajorPeak(vReal, samples, samplingFrequency);
  // Serial.print("major peak: ");
  // Serial.println(x);
  //while (1)
    ; /* Run Once */
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

void SplitSampleNormalized(double *vData, unsigned int bufferSize, int SplitInto) {
  unsigned int newBufferSize = normalizeArray2(vData, vRealNormalized, bufferSize);
  
  // for (int i = 0; i < SplitInto; i++) {

  // }
}

// Splits the samples into x even groups, works best when bufferSize and splitInto are a power of 2.
void SplitSample(double *vData, uint16_t bufferSize, int splitInto) {

  int splitRange = floor(bufferSize / splitInto);

  double splitMeanArray[splitInto];

  for (int i = 0; i < splitInto; i++) {
    double amplitudeGroup[splitRange];
    for (int j = 0; j < splitRange; j++) {
      uint16_t k = j + i * splitRange;
      // if amplitude is above certain threshold, set it to -1
      if (vData[k] > OUTLIER) {
        amplitudeGroup[j] = -1;
      } else {
        amplitudeGroup[j] = vData[k];
      }
    }
    splitMeanArray[i] = getArrayMean(amplitudeGroup, splitRange);
  }

  double normalizedArray[splitInto];

  normalizeArray(splitMeanArray, normalizedArray, splitInto);

  int freqRange = samplingFrequency / splitInto;

  PrintNormalizedArray(normalizedArray, splitInto, freqRange);
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
unsigned int normalizeArray2(double *array, double *destArray, int arraySize) {
  double min = getArrayMin2(array, arraySize);
  double max = getArrayMax2(array, arraySize);

  double iv = 1.0 / (max - min);
  unsigned int size = 0;
  for (int i = 0; i < arraySize; i++) {
    if (array[i] < OUTLIER) {
      double normalizedValue = (array[i] - min) * iv;
      destArray[size] = normalizedValue;
      size += 1;
    }
  }
  return size;
}

// normalizes array and puts the normalized values in destArray
void normalizeArray(double *array, double *destArray, int arraySize) {
  double min = getArrayMin(array, arraySize);
  double max = getArrayMax(array, arraySize);

  for (int i = 0; i < arraySize; i++) {
    double normalizedValue = (array[i] - min) / (max - min);
    destArray[i] = normalizedValue;
  }
}

// returns the minimum value in array
double getArrayMin2(double *array, int arraySize) {
  double min = OUTLIER;
  for (int i = 0; i < arraySize; i++) {
    if (array[i] < OUTLIER && array[i] > 0 && array[i] < min) {
      min = array[i];
    }
  }
  return min;
}

// returns the maximum value in array
double getArrayMax2(double *array, int arraySize) {
  double max = 0;
  for (int i = 0; i < arraySize; i++) {
    if (array[i] < OUTLIER && array[i] > max) {
      max = array[i];
    }
  }
  return max;
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


// prints normalized amplitudes
void PrintNormalizedArray(double *array, int arraySize, int freqRange) {
  Serial.println("\nPrinting normalized amplitudes:");
  for (int i = 0; i < arraySize; i++) {
    char buffer[64];
    // char normalizedAmplitude = char(array[i]);
    int rangeLow = i * freqRange;
    int rangeHigh = rangeLow + freqRange;
    // Serial.print(array[i], 2);
    // Serial.print(" for freq range between ");
    // Serial.print(rangeLow);
    // Serial.print("Hz and ");
    // Serial.print(rangeHigh);
    // Serial.println("Hz\n\n\n\n");
    sprintf(buffer, "%.3f for frequency range between %d and %d Hz\n", array[i], rangeLow, rangeHigh);
    Serial.print(buffer);
  }
}