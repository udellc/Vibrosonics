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

// Enums for the config fields for real-time updates
enum class ConfigField : uint
{
  // Global
  NoiseFloor = 0u,
  CfarRefCount,
  CfarGuardCount,
  CfarBias,
  SmoothingFactor,

  // Marker for global fields
  GLOBAL_END = SmoothingFactor,

  // Shared module fields
  FreqLow,
  FreqHigh,
  OutputNumber,
  MinAmpNorm,

  // MajorPeaks
  MaxPeaks,
  FrequencyMapping,

  // Percussion
  FluxThresh,
  EnergyThresh,
  EntropyThresh,
  WaveType
};

enum FrequencyMapping{
  NONE = 0,
  OCTAVE,
  MIDI
};

// note: add more and update sketch as more support is added
enum ModuleType{
  MAJORPEAKS = 0,
  PERCUSSION
};

// configuration for a single analysis module. constructor is protected as we only
// want to instantiate derived structs
struct ModuleConfig {
  virtual ~ModuleConfig() = default;
protected:
  ModuleConfig(
              int outputNumber,
              ModuleType moduleType,
              uint16_t freqLow,
              uint16_t freqHigh,
              float minAmpNorm)
    : outputNumber(outputNumber),
      moduleType(moduleType),
      freqLow(freqLow),
      freqHigh(freqHigh),
      minAmpNorm(minAmpNorm) {}
public:
  // the output number that the module is assigned to
  int outputNumber;
  // type of the analysis module. see ModuleType enum for options
  const ModuleType moduleType;
  // low value of the frequency range to pick up. must be positive 
  uint16_t freqLow;
  // high value of the frequency range to pick up. must be positive and should be larger than freqLow
  uint16_t freqHigh;
  // minimum value to use for amplitude mapping. must be positive
  float minAmpNorm;
};

// configuration unique to the major peaks analysis module
struct MajorPeaksConfig : ModuleConfig {
  MajorPeaksConfig(int outputNumber,
                  uint16_t freqLow,
                  uint16_t freqHigh,
                  float minAmpNorm,
                  FrequencyMapping frequencyMapping,
                  int maxPeaks)
    : ModuleConfig(outputNumber, MAJORPEAKS, freqLow, freqHigh, minAmpNorm),
      maxPeaks(maxPeaks),
      frequencyMapping(frequencyMapping) {}
  // what type of frequency mapping to use. see FrequencyMapping enum for options
  FrequencyMapping frequencyMapping;
  // number of peaks to pick up in analysis. minimum of 1
  int maxPeaks;
};

// configuration unique to the percussion analysis module
struct PercussionConfig : ModuleConfig {
  PercussionConfig(int outputNumber,
                  uint16_t freqLow,
                  uint16_t freqHigh,
                  float minAmpNorm,
                  float fluxThresh,
                  float energyThresh,
                  float entropyThresh,
                  WaveType waveType)
    : ModuleConfig(outputNumber, PERCUSSION, freqLow, freqHigh, minAmpNorm),
      fluxThresh(fluxThresh),
      energyThresh(energyThresh),
      entropyThresh(entropyThresh),
      waveType(waveType) {}
  float fluxThresh;
  float energyThresh;
  float entropyThresh;
  WaveType waveType;
};

/*
Storing the modules configs using a unique_ptr, since the current method of updating settings
is using a pointer swap with shared_ptr. The logic here is that once the web server loop creates a
new shared_ptr instance of the AnalysisConfig for the audio loop, the old AnalysisConfig is deleted, which
then rolls over to the unique_ptrs since they no longer have any owners in scope.
*/
using ModulePtr = std::unique_ptr<ModuleConfig>;

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

  ModulePtr modules[NUM_OUT_CH] = { nullptr };
};

#endif