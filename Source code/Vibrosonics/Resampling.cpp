#include "Vibrosonics.h"

float *downsampleFilterTable = NULL;
int filterTableIdx = 0;
int downsampleFilterTableSize = 0;

int dowsampleRatio = 0;

int* downsampleFilterInput = NULL;
int downsampleInputIdx = 0;

int downsampleFilterOutput[WINDOW_SIZE];
int downsampleOutputIdx = 0;

void Vibrosonics::calculateDownsampleSincFilterTable(int ratio, int nz) {
  dowsampleRatio = ratio;
  downsampleFilterTableSize = (2*nz + 1) * ratio - ratio + 1;

  downsampleFilterTable = (float*)malloc(sizeof(float) * downsampleFilterTableSize);
  downsampleFilterInput = (int*)malloc(sizeof(int) * downsampleFilterTableSize);

  float ns[downsampleFilterTableSize];
  float ns_step = float(nz * ratio * 2) / (downsampleFilterTableSize - 1);

  float t[downsampleFilterTableSize];
  float t_step = 1.0 / (downsampleFilterTableSize - 1);

  for (int i = 0; i < downsampleFilterTableSize; i++) {
    ns[i] = float(-1.0 * nz * ratio) + ns_step * i;
    t[i] = t_step * i;
  }

  // calculate sinc filter table
  for (int i = 0; i < downsampleFilterTableSize; i++) {
    downsampleFilterTable[i] = (1.0 / ratio) * sin(PI * ns[i] / ratio) / (PI * ns[i] / ratio);
    downsampleFilterInput[i] = 0;
  }

  downsampleFilterTable[int(round((downsampleFilterTableSize - 1) / 2.0))] = 1.0 / ratio;

  // window sinc filter table with a cosine wave
  for (int i = 0; i < downsampleFilterTableSize; i++) {
    downsampleFilterTable[i] = downsampleFilterTable[i] * 0.5 * (1.0 - cos(2.0 * PI * t[i]));
  }
}

bool Vibrosonics::downsampleSignal(int* signal) {
  int signalIdx = 0;

  // temporary index to remeber the filters last position
  while(signalIdx < WINDOW_SIZE) {
    int sincFilterTableIdxCpy = filterTableIdx;
    for (int i = 0; i < dowsampleRatio; i++) {
      downsampleFilterInput[filterTableIdx++] = signal[signalIdx++];

      if (filterTableIdx == downsampleFilterTableSize) filterTableIdx = 0;
    }

    // calculate downsampled value
    float downsampledValue = 0.0;
    for (int i = 0; i < downsampleFilterTableSize; i++) {
      downsampledValue += downsampleFilterInput[sincFilterTableIdxCpy++] * downsampleFilterTable[i];
      if (sincFilterTableIdxCpy == downsampleFilterTableSize) sincFilterTableIdxCpy = 0;
    }
    downsampleFilterOutput[downsampleOutputIdx++] = downsampledValue;
  }
  // return true when downsample output buffer fills
  if (downsampleOutputIdx == WINDOW_SIZE) {
    downsampleOutputIdx = 0;
    return 1;
  }
  return 0;
}

int *Vibrosonics::getDownsampledSignal(void) {
  return downsampleFilterOutput;
}