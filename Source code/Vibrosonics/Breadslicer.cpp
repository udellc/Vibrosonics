#include "Breadslicer.h"

Breadslicer::Breadslicer(int aWindowSize, int aSampleRate) {
  this->windowSize = aWindowSize;
  this->sampleRate = aSampleRate;

  this->frequencyWidth = this->windowSize * 1.0 / this->sampleRate;

  this->numBands = 0;

  this->bandIndexes = NULL;
  this->output = NULL;
}

Breadslicer::~Breadslicer() {
  if (this->bandIndexes != NULL) free(bandIndexes);
  if (this->output != NULL) free(output);
}

// setBands([0, 200, 500, 2000, 4000])
void Breadslicer::setBands(int *frequencyBands, int numBands) {

  int _nyquist = this->sampleRate >> 1;

  for (int i = 0; i < numBands; i++) {
    if (!((frequencyBands[i] >= 0 && frequencyBands[i] <= _nyquist) &&
        (frequencyBands[i] < frequencyBands[i + 1] && frequencyBands[i + 1] <= _nyquist)))
    {
      Serial.println("Breadslicer setBands() fail! Invalid bands!");
      return;
    }
  }

  if (this->bandIndexes != NULL) free(bandIndexes);
  if (this->output != NULL) free(output);

  this->bandIndexes = (int*)malloc(sizeof(int) * numBands + 1);
  this->output = (float*)malloc(sizeof(float) * numBands);
  this->numBands = numBands;

  for (int i = 0; i < numBands + 1; i++) {
    bandIndexes[i] = round(frequencyBands[i] * this->frequencyWidth);
    if (i < numBands) {
      this->output[i] = 0.0;
    }
  }
}

void Breadslicer::perform(float *input) {
  if (this->bandIndexes == NULL) return;

  int _bandIndex = 1;
  float _bandSum = 0;
  for (int i = this->bandIndexes[0]; i <= this->bandIndexes[this->numBands]; i++) {
    if (i < this->bandIndexes[_bandIndex]) {
      _bandSum += input[i];
    }
    else {
      this->output[_bandIndex - 1] = _bandSum;
      _bandIndex += 1;
      _bandSum = 0;
      i -= 1;
    }
  }
}

float *Breadslicer::getOutput() {
  return this->output;
}