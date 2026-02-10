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

struct OutputConfig {
  virtual ~OutputConfig() = default;
  OutputConfig(ModuleType moduleType,
               uint16_t freqLow,
               uint16_t freqHigh,
               FrequencyMapping frequencyMapping,
               float minAmpNorm)
    : moduleType(moduleType),
      freqLow(freqLow),
      freqHigh(freqHigh),
      frequencyMapping(frequencyMapping),
      minAmpNorm(minAmpNorm) {}
  ModuleType moduleType = EMPTY;
  uint16_t freqLow;
  uint16_t freqHigh;
  FrequencyMapping frequencyMapping;
  float minAmpNorm;
};

struct MajorPeaksConfig : OutputConfig {
  MajorPeaksConfig(uint16_t freqLow,
                   uint16_t freqHigh,
                   FrequencyMapping frequencyMapping,
                   float minAmpNorm,
                   int maxPeaks)
    : OutputConfig(MAJORPEAKS, freqLow, freqHigh, frequencyMapping, minAmpNorm),
      maxPeaks(maxPeaks) {}
  int maxPeaks;
};

// TODO: add percussion config as a child of OutputConfig

struct AnalysisConfig {
  float noiseFloor = 280;
  uint16_t cfarRefCount = 6;
  uint16_t cfarGuardCount = 1;
  float cfarBias = 1.4;
  float smoothingFactor = 0.3;
  OutputConfig* outputs[NUM_OUT_CH];
} ;

#endif