/***************************************************************
 * FILE: storage.h
 * 
 * DATE: 2/12/2026
 * 
 * DESCRIPTION: This namespace contains structs and enums needed
 * to store analysis configurations for Configurable.ino
 * 
 * AUTHOR: Danielle Chang
 ***************************************************************/

#ifndef STORAGE_H
#define STORAGE_H

#include <AudioLab.h>
#include <memory>

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

// configuration for a single analysis module. constructor is protected as we only
// want to instantiate derived structs
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
  // type of the analysis module. see ModuleType enum for options
  const ModuleType moduleType;
  // low value of the frequency range to pick up. must be positive 
  uint16_t freqLow;
  // high value of the frequency range to pick up. must be positive and should be larger than freqLow
  uint16_t freqHigh;
  // what type of frequency mapping to use. see FrequencyMapping enum for options
  FrequencyMapping frequencyMapping;
  // minimum value to use for amplitude mapping. must be positive
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
  // number of peaks to pick up in analysis. minimum of 1
  int maxPeaks;
};

// TODO: add percussion config as a child of ModuleConfig

// configuration for our entire analysis
struct AnalysisConfig {
  // frequency to use for the noise floor. minimum of 0
  float noiseFloor = 280;
  // number of reference cells to use for CFAR noise cancellation. minimum of 1
  uint16_t cfarRefCount = 6;
  // number of guard cells to use for CFAR noise cancellation algorithm. minimum of 1
  uint16_t cfarGuardCount = 1;
  // bias to use for CFAR noise cancellation algorithm. minimum of 0 (no CFAR)
  float cfarBias = 1.4;
  // smoothing factor for smooth_window_over_time. value from 0-1 (0 high smoothing, 1 no smoothing)
  float smoothingFactor = 0.3;
  std::unique_ptr<ModuleConfig> modules[NUM_OUT_CH];
};

#endif