#ifndef STORAGE_H
#define STORAGE_H

#include <AudioLab.h>

enum FrequencyMapping{
  NONE,
  OCTAVE,
  MIDI
};

// note: add more and update sketch as more support is added
enum ModuleType{
  EMPTY,
  MAJORPEAKS,
  PERCUSSION
};

typedef struct {
  ModuleType moduleType = EMPTY;
  uint16_t freqLow;
  uint16_t freqHigh;
  FrequencyMapping frequencyMapping;
  float minAmpNorm;
} OutputConfig;

typedef struct {
  float noiseFloor = 280;
  uint16_t cfarRefCount = 6;
  uint16_t cfarGuardCount = 1;
  float cfarBias = 1.4;
  float smoothingFactor = 0.3;
  OutputConfig outputs[NUM_OUT_CH];
} AnalysisConfig;

#endif