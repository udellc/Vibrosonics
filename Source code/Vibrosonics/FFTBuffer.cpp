#include "FFTBuffer.h"

FFTBuffer::FFTBuffer(int aWindowSize, int aSampleRate, int aNumberOfWindows) {
  this->windowSize = aWindowSize;
  this->sampleRate = aSampleRate;
  this->bufferSize = aNumberOfWindows;
  this->frequencyWidth = this->windowSize * 1.0 / this->sampleRate;

  this->timeFrequencyWindowCount = 0;

  this->timeFrequencyWindows = (float**)malloc(sizeof(float*) * aNumberOfWindows);

  for (int i = 0; i < this->bufferSize; i++) {
    this->timeFrequencyWindows[i] = (float*)malloc(sizeof(float) * aWindowSize);
    for (int j = 0; j < this->windowSize; j++) {
      this->timeFrequencyWindows[i][j] = 0.0;
    }
  }
}

FFTBuffer::~FFTBuffer(void) {
  for (int i = 0; i < this->bufferSize; i++) {
    free(this->timeFrequencyWindows[i]);
  }
  free(timeFrequencyWindows);
}

void FFTBuffer::pushData(float *someFFTData) {
  this->timeFrequencyWindowCount += 1;
  if (this->timeFrequencyWindowCount == this->bufferSize) timeFrequencyWindowCount = 0;

  for (int i = 0; i < this->windowSize >> 1; i++) {
    this->timeFrequencyWindows[this->timeFrequencyWindowCount][i] = someFFTData[i] * this->frequencyWidth;
  }
}

// returns double pointer to FFT data buffer
float **FFTBuffer::getData(void) {
  return this->timeFrequencyWindows;
}

float *FFTBuffer::getWindow(int aWindowIndex) {
  if (aWindowIndex == this->bufferSize) return timeFrequencyWindows[0];
  else if (aWindowIndex == -1) return timeFrequencyWindows[7];
  else if (aWindowIndex > this->bufferSize) return timeFrequencyWindows[aWindowIndex % this->bufferSize];
  else if (aWindowIndex < -1) {
    int windowIndex = this->bufferSize - (abs(aWindowIndex) % this->bufferSize);
    if (windowIndex = this->bufferSize) return timeFrequencyWindows[0];
    return timeFrequencyWindows[this->bufferSize - windowIndex];
  }

  return this->timeFrequencyWindows[aWindowIndex];
}

int FFTBuffer::getCurrentWindowIndex(void) {
  return this->timeFrequencyWindowCount;
}
