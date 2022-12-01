// VibroSonics FFT Hardcoded Bins
// Samson DeVol, 11/10/22
// Copyright (C) 2018 Enrique Condés and Ragnar Ranøyen Homb

#include <arduino.h>
#include "arduinoFFT.h"

#define CHANNEL A2
#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

arduinoFFT FFT = arduinoFFT(); /* Create FFT object */
const uint16_t SAMPLES = 512; /* This value MUST ALWAYS be a power of 2 */
const double SIGNAL_FREQUENCY = 1000;
const double SAMPLING_FREQUENCY = 8192;
const uint8_t AMPLITUDE = 100;
const int NUMBER_OF_BINS = 7;
const int HARDCODED_BINS[NUMBER_OF_BINS] = {0, 250, 500, 1000, 2000, 4000, 8000};

double vReal[SAMPLES];
double vImag[SAMPLES];

unsigned int sampling_period_us;
unsigned long microseconds;

void setup(){
  sampling_period_us = round(1000000*(1.0/SAMPLING_FREQUENCY)); /* set sampling period */
  Serial.begin(115200);   /*set baud rate, 115200 preferred */
  while(!Serial);
  Serial.println("Ready");
}

void loop() {
  microseconds = micros(); /* microseconds since program began */
  for(int i=0; i<SAMPLES; i++)
  {
      vReal[i] = analogRead(CHANNEL);  /* read pcm audio into vreal array */
      vImag[i] = 0; /* create vimag array of zeros with equal length */
      while(micros() - microseconds < sampling_period_us){
        //empty loop
      }
      microseconds += sampling_period_us;
  }
  
  // compute fft 
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);	/* Weigh data */
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD); /* Compute FFT */
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES); /* Compute magnitudes */

  splitSampleHardcoded(vReal); /* split sample into frequency range bins */

  //while(1); /* Run Once */
  delay(10000); /* Repeat after delay */
}

void splitSampleHardcoded(double *vData) {

  double bin[SAMPLES] = {0}; /* lists of doubles with length equal to number of samples */

  for (int i=0; i < NUMBER_OF_BINS - 1; i++){ /* loop for each bin depending on number of hardcoded bins */
    memset(bin, 0, sizeof(bin)); /* set current bin array to all zeros */
    int bin_index = 0; /* set value for current bin index to zero */

    for (int j = 0; j < SAMPLES; j++){ /* loop for each sampled value in the recorded data */
      if (vData[j] > HARDCODED_BINS[i] and vData[j] < HARDCODED_BINS[i+1]){ /* if sampled value falls within current bin range add to array */
        bin[bin_index] = vData[j];
        bin_index++;
      }
    }
    printFrequencyArray(bin, bin_index, i);
  }
}

void printFrequencyArray(double *array, int index, int i_val) {
  Serial.print("For Frequency Range ");
  Serial.print(HARDCODED_BINS[i_val]);
  Serial.print(" - ");
  Serial.print(HARDCODED_BINS[i_val+1]);
  Serial.print("Hz");
  Serial.print(" Has Mean: ");
  Serial.print(getArrayMean(array, index));
  Serial.print("\n");
}

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

void normalizeArray(double *array, double *destArray, int arraySize) {
  double min = getArrayMin(array, arraySize);
  double max = getArrayMax(array, arraySize);

  for (int i = 0; i < arraySize; i++) {
    double normalizedValue = (array[i] - min) / (max - min);
    destArray[i] = normalizedValue;
  }
}

double getArrayMin(double *array, int arraySize) {
  double min = array[0];
  for (int i = 1; i < arraySize; i++) {
    if (array[i] < min) {
      min = array[i];
    }
  }
  return min;
}

double getArrayMax(double *array, int arraySize) {
  double max = array[0];
  for (int i = 1; i < arraySize; i++) {
    if (array[i] > max) {
      max = array[i];
    }
  }
  return max;
}
