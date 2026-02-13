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
  MAJORPEAKS,
  PERCUSSION
};

// configuration for a single analysis module
struct ModuleConfig {
  virtual ~ModuleConfig() = default;
protected:
  ModuleConfig(ModuleType moduleType,
               uint16_t freqLow,
               uint16_t freqHigh,
               FrequencyMapping frequencyMapping,
               float minAmpNorm)
    : moduleType(moduleType),
      freqLow(freqLow),
      freqHigh(freqHigh),
      frequencyMapping(frequencyMapping),
      minAmpNorm(minAmpNorm) {}
public:
  const ModuleType moduleType;
  uint16_t freqLow;
  uint16_t freqHigh;
  FrequencyMapping frequencyMapping;
  float minAmpNorm;
};

// configuration unique to the major peaks analysis module
struct MajorPeaksConfig : ModuleConfig {
  MajorPeaksConfig(uint16_t freqLow,
                   uint16_t freqHigh,
                   FrequencyMapping frequencyMapping,
                   float minAmpNorm,
                   int maxPeaks)
    : ModuleConfig(MAJORPEAKS, freqLow, freqHigh, frequencyMapping, minAmpNorm),
      maxPeaks(maxPeaks) {}
  int maxPeaks;
};

// TODO: add percussion config as a child of ModuleConfig

// configuration for our entire analysis
struct AnalysisConfig {
  float noiseFloor = 280;
  uint16_t cfarRefCount = 6;
  uint16_t cfarGuardCount = 1;
  float cfarBias = 1.4;
  float smoothingFactor = 0.3;
  ModuleConfig* modules[NUM_OUT_CH];
} ;

#endif