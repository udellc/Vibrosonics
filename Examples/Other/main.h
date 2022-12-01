// VibroSonics header file 
// Copyright (C) 2018 Enrique Condés and Ragnar Ranøyen Homb

// from hardcoded fft - Samson
#include <arduino.h>
#include "arduinoFFT.h"
#define CHANNEL A2
#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03
arduinoFFT FFT = arduinoFFT(); 
const uint16_t SAMPLES = 512;
const double SIGNAL_FREQUENCY = 1000;
const double SAMPLING_FREQUENCY = 8192;
const uint8_t AMPLITUDE = 100;
const int NUMBER_OF_BINS = 7;
const int HARDCODED_BINS[NUMBER_OF_BINS] = {0, 250, 500, 1000, 2000, 4000, 8000};
double vReal[SAMPLES];
double vImag[SAMPLES];
unsigned int sampling_period_us;
unsigned long microseconds;

// from add synth - Garrett
#include <arduinoFFT.h>
#include <Arduino.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>
#define CONTROL_RATE 128
Oscil <2048, AUDIO_RATE> asinc(SIN2048_DATA);
Oscil <2048, AUDIO_RATE> asin1(SIN2048_DATA);
Oscil <2048, AUDIO_RATE> asin2(SIN2048_DATA);
Oscil <2048, AUDIO_RATE> asin3(SIN2048_DATA);
const int OscilCount = 4; 
int gainzDivisor;
float sickGainz[OscilCount] = { 0 };
int realGainz[OscilCount] = { 0 };  
long currentCarrier;
long nextCarrier;

// from parabolic bins - Nick
#include <arduino.h>
#include "arduinoFFT.h"
#define CHANNEL A2
arduinoFFT FFT = arduinoFFT();
const uint16_t samples = 512;
const double samplingFrequency = 8192;
const double OUTLIER = 5000.0;
double vReal[samples];
double vImag[samples];
double vRealNormalized[samples];
unsigned int sampling_period_us;
unsigned long microseconds;
#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

// from hardcoded bins - Samson
void splitSample(double *vData);
void printFrequencyArray(double *array, int index, int i_val);
double getArrayMean(double *array, int arraySize);
void normalizeArray(double *array, double *destArray, int arraySize);
double getArrayMin(double *array, int arraySize);
double getArrayMax(double *array, int arraySize);
void print_array(double *array, int arraySize, int freqRange);

// from add synth - Garrett
void sickGainzTranslation(float gainz[]);
void updateControl();
void gainzControl();
int updateAudio();

// from parabolic bins - Nick
void PrintVector(double *vData, uint16_t bufferSize, uint8_t scaleType);
void SplitSample(double *vData, uint16_t bufferSize, int splitInto, double curveValue);
double getArrayMean(double *array, int arraySize);
void normalizeArray(double *array, double *destArray, int arraySize);
double getArrayMin(double *array, int arraySize);
double getArrayMax(double *array, int arraySize);
void PrintArray(double *array, int arraySize, double curveValue);
