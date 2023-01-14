#ifndef ESP32FFTTOPARABOLICBINS_H
#define ESP32FFTTOPARABOLICBINS_H

void FFTtoParaSetup();
void FFTtoParaLoop();
void PrintVector(double *vData, uint16_t bufferSize, uint8_t scaleType);
void SplitSample(double *vData, uint16_t bufferSize, int splitInto, double curveValue);
double getArrayMean(double *array, int arraySize);
void normalizeArray(double *array, double *destArray, int arraySize);
double getArrayMin(double *array, int arraySize);
double getArrayMax(double *array, int arraySize);
void PrintArray(double *array, int arraySize, double curveValue);

#endif