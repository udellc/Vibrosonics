#include "Vibrosonics.h"

float *downsampleFilterTable = NULL;
int filterTableIdx = 0;
int filterTableSize = 0;

int dowsampleRatio = 0;

int* downsampleFilterInput = NULL;
int downsampleInputIdx = 0;

int downsampleFilterOutput[WINDOW_SIZE];
int downsampleOutputIdx = 0;

void Vibrosonics::calculateDownsampleSincFilterTable(int ratio, int nz) {
  dowsampleRatio = ratio;
  int n = (2*nz + 1) * ratio - ratio + 1;

  float ns[n];
  float ns_step = float(nz * ratio * 2) / (n - 1);

  float t[n];
  float t_step = 1.0 / (n - 1);

  for (int i = 0; i < n; i++) {
    ns[i] = float(-1.0 * nz * ratio) + ns_step * i;
    t[i] = t_step * i;
  }

  filterTableSize = n;
  downsampleFilterTable = (float*)malloc(sizeof(float) * filterTableSize);
  downsampleFilterInput = (int*)malloc(sizeof(int) * filterTableSize);

  for (int i = 0; i < filterTableSize; i++) {
    downsampleFilterTable[i] = (1.0 / ratio) * sin(PI * ns[i] / ratio) / (PI * ns[i] / ratio);
    downsampleFilterInput[i] = 0;
  }

  downsampleFilterTable[int(round((n - 1) / 2.0))] = 1.0 / ratio;
  for (int i = 0; i < filterTableSize; i++) {
    downsampleFilterTable[i] = downsampleFilterTable[i] * 0.5 * (1.0 - cos(2.0 * PI * t[i]));
  }
}

bool Vibrosonics::downsampleSignal(int* signal) {
  int signalIdx = 0;

  int sincFilterTableIdxCpy = filterTableIdx;
  while(signalIdx < WINDOW_SIZE) {
    for (int i = 0; i < dowsampleRatio; i++) {
      sincFilterTableIdxCpy = filterTableIdx;
      downsampleFilterInput[filterTableIdx++] = signal[signalIdx++];

      if (filterTableIdx == filterTableSize) filterTableIdx = 0;
    }

    float downsampledValue = 0.0;
    for (int i = 0; i < filterTableSize; i++) {
      downsampledValue += downsampleFilterInput[sincFilterTableIdxCpy++] * downsampleFilterTable[i];
      if (sincFilterTableIdxCpy == filterTableSize) sincFilterTableIdxCpy = 0;
    }
    downsampleFilterOutput[downsampleOutputIdx++] = downsampledValue;
  }
  if (downsampleOutputIdx == WINDOW_SIZE) {
    downsampleOutputIdx = 0;
    return 1;
  }
  return 0;
}

int *Vibrosonics::getDownsampledSignal(void) {
  return downsampleFilterOutput;
}