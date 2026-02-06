#ifndef STORAGE_H
#define STORAGE_H

#include <AudioLab.h>

enum FrequencyMapping{
  NONE,
  OCTAVE,
  MIDI
};

typedef struct {
  uint8_t freqLow;
  uint8_t freqHigh;
  float minAmpNorm;
} OutputConfig;

typedef struct {
  float noiseFloor = 0;
  uint8_t cfarRefCount = 6;
  uint8_t cfarGuardCount = 1;
  float cfarBias = 1.4;
  float smoothingFactor = 0.3;
  FrequencyMapping frequencyMapping = OCTAVE;
  OutputConfig outputs[NUM_OUT_CH];
} AnalysisConfig;

#endif