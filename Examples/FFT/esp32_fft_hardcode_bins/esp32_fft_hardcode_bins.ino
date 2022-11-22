// VibroSonics FFT Demonstration Code
// Samson DeVol, 11/10/22
// Copyright (C) 2018 Enrique Condés and Ragnar Ranøyen Homb

#include <arduino.h>
#include "arduinoFFT.h"

#define CHANNEL A2
#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

// decalre globals
arduinoFFT FFT = arduinoFFT(); /* Create FFT object */
const uint16_t SAMPLES = 512; //This value MUST ALWAYS be a power of 2
const double SIGNAL_FREQUENCY = 1000;
const double SAMPLING_FREQUENCY = 8192;
const uint8_t AMPLITUDE = 100;

double vReal[SAMPLES];
double vImag[SAMPLES];

unsigned int sampling_period_us;
unsigned long microseconds;

void setup()
{
  // set sampling period
  sampling_period_us = round(1000000*(1.0/SAMPLING_FREQUENCY));

  // set baud rate, 115200 preferred
  Serial.begin(115200);
  while(!Serial);
  Serial.println("Ready");
}

void loop()
{

  // microseconds since program began
  microseconds = micros();

  // read pcm audio into vreal array
  // create vimag array of zeros with equal length
  for(int i=0; i<SAMPLES; i++)
  {
      vReal[i] = analogRead(CHANNEL);
      vImag[i] = 0;
      while(micros() - microseconds < sampling_period_us){
        //empty loop
      }
      microseconds += sampling_period_us;
  }
  
  // compute fft 
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);	/* Weigh data */
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD); /* Compute FFT */
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES); /* Compute magnitudes */

  // split sample into frequency range bins
  SplitSample(vReal,  5);

  //while(1); /* Run Once */
  delay(10000); /* Repeat after delay */
}

void FindPeak(double *vData, uint16_t SAMPLES, double SAMPLING_FREQUENCY) {
/* find peak
  double x = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);
  Serial.print("major peak: ");
  Serial.println(x);
*/ 
}

void SplitSample(double *vData, int bin_num) {

  // evenly split bin sizes for number of SAMPLES
  int bin_size = ceil(SAMPLES / bin_num);

  // range of each frequency per bin
  int frequency_range_per_bin = SAMPLING_FREQUENCY / bin_num;

  // lists of doubles with length equal to number of bins
  double mean_value_bins[5];
  double bins[5][512];
  double lowest_freq[512];
  double second_freq[512];
  int frequency_groupings[6] = {250, 500, 1000, 2000, 4000, 8000};

  // counters for each frequency range
  int j=0;
  int k=0;
  int l=0;
  int m=0;
  int n=0;
  int o=0;
  for (int i = 0; i < SAMPLES; i++){

    /* print sample data
    // Serial.print("di: ");
    // Serial.print(i);
    // Serial.print(" ");
    // Serial.print(vData[i]);
    // Serial.print("\n");
    */

    if (vData[i] < frequency_groupings[0] and vData[i] > 50){
      //bins[0][j] = vData[i];
      lowest_freq[j] = vData[i];
      j++;
    }
    else if (vData[i] < frequency_groupings[1]){
      //bins[1][k] = vData[i];
      second_freq[k] = vData[i];
      k++;
    }
    else if (vData[i] < frequency_groupings[2]){
      //bins[2][l] = vData[i];
      l++;
    }
    else if (vData[i] < frequency_groupings[3]){
      //bins[3][m] = vData[i];
      m++;
    }
    else if (vData[i] < frequency_groupings[4]){
      //bins[4][n] = vData[i];
      n++;
    }
    else if (vData[i] < frequency_groupings[4]){
      //bins[4][n] = vData[i];
      o++;
    }

  }
  double normalizedArray[512];
  
  //normalizeArray(lowest_freq, normalizedArray, 512);
  //print_array(normalizedArray, 512, 250);

  Serial.print("****************\n");
  Serial.print("Number of Samples in the range 50Hz - 250Hz: ");
  Serial.print(j);
  Serial.print(" with mean amplitude: ");
  Serial.print(getArrayMean(lowest_freq, j));
  Serial.print("\n");
  Serial.print("Number of Samples in the range 250Hz - 500Hz: ");
  Serial.print(k);
  //Serial.print(" with mean amplitude: ");
  //getArrayMean(second_freq, k);
  Serial.print("\n");
  Serial.print("Number of Samples in the range 500Hz - 1000Hz: ");
  Serial.print(l);
  Serial.print("\n");
  Serial.print("Number of Samples in the range 1000Hz - 2000Hz: ");
  Serial.print(m);
  Serial.print("\n");
  Serial.print("Number of Samples in the range 2000Hz - 4000Hz: ");
  Serial.print(n);
  Serial.print("\n");
  Serial.print("Number of Samples in the range 4000Hz - 8000Hz: ");
  Serial.print(o);
  Serial.print("\n");
  Serial.print("****************\n");

  // for (int ii=0; ii < 5; ii++){
  //   Serial.print("Get mean from ");
  //   Serial.print(ii);
  //   Serial.print(bins[0][0]);
  //   Serial.print(" ");

  //   Serial.print(" ");
  //   double cur_sum = 0;
  //   for (int jj=0; jj < 20; jj++){
  //     Serial.print(bins[ii][jj]);
  //     //cur_sum += bins[ii][jj];
  //   }
  //   Serial.print(cur_sum);
  //   Serial.print("\n");
  // }



  // for each bin
  // for (int i = 0; i < bin_num; i++) {

  //   // array of doubles for grouping
  //   double amplitudeGroup[bin_size];
  //   Serial.print("ampG size: ");
  //   Serial.print(sizeof(amplitudeGroup)/sizeof(double));
  //   Serial.print("\n");

  //   Serial.print("ampG size: ");
  //   Serial.print(sizeof(amplitudeGroup)/sizeof(double));
  //   Serial.print("\n");
  //   for (int j = 0; j < bin_size; j++) {
      
  //     // first bin
  //     if (i == 0 and j < 8 ) {
  //       amplitudeGroup[j] = 0;
  //     }
  //     // last bin
  //     else if (i == (bin_num - 1) and j > (bin_size - 16)) {
  //       amplitudeGroup[j] = 0;
  //     }
  //     else {
  //       // current point 
  //       uint16_t k = j + i * bin_size;
  //       // Serial.print("di: ");
  //       // Serial.print(k);
  //       // Serial.print(" ");
  //       // Serial.print(vData[k]);
  //       // Serial.print("\n");
  //       // amplitudeGroup[j] = vData[k];
  //     }
  //   }
    
  //   // for bin, get mean value
  //   mean_value_bins[i] = getArrayMean(amplitudeGroup, bin_size);
    
  // }

  
  // normalizeArray(mean_value_bins, normalized_mean_value_bins, bin_num);
  // print_array(normalized_mean_value_bins, bin_num, frequency_range_per_bin);
}

// double get_total(double *array){
//   double total = 0;
//   int value_count=0;
//   for (int i = 0; i < 512; i++) {
//     Serial.print("di: ");
//     Serial.print(i);
//     Serial.print(" ");
//     Serial.print(array[i]);
//     Serial.print("\n");
//   }
//   return (value_count);
// }

double getArrayMean(double *array, int arraySize) {
  double arraySum = 0;
  for (int i = 0; i < arraySize; i++) {
    arraySum += array[i];
  }
 // Serial.print(arraySum);
 // Serial.print("/");
 // Serial.print(arraySize);
  return (arraySum / arraySize);
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

void print_array(double *array, int arraySize, int freqRange) {
  Serial.println("\nPrinting normalized amplitudes:");
  for (int i = 0; i < arraySize; i++) {
    char buffer[64];
    int rangeLow = i * freqRange;
    int rangeHigh = rangeLow + freqRange;
    sprintf(buffer, "%2f for frequency range between %d and %d Hz\n", array[i], rangeLow, rangeHigh);
    Serial.print(buffer);
  }
}

