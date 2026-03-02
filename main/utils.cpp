/***************************************************************
 * FILE: utils.cpp
 * 
 * DATE: 2/19/2026
 * 
 * DESCRIPTION: Implementation file for the Utils namespace.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#include "config.h"
#include "storage.h"
#include <map>
#include <functional>
#include <memory>
#include "utils.h"

using CreateModule = std::function<ModulePtr()>;
using ModuleFactory = std::map<ModuleType, CreateModule>;

/**
 * @brief Populates the global analysis configuration settings using
 *        the JSON structure found in /WebApp/src/data.
 * 
 * @param global - Reference to the JSON object to get the global data from.
 * @param config - Pointer to the AnalysisConfig to update.
 */
void Utils::populateGlobalSettings(JsonObject& global, AnalysisConfig* config)
{
  config->noiseFloor = global["noiseFloor"] | 280.0f;
  config->cfarRefCount = global["cfarRefCount"] | 6;
  config->cfarGuardCount = global["cfarGuardCount"] | 1;
  config->cfarBias = global["cfarBias"] | 1.2f;
  config->smoothingFactor = global["smoothingFactor"] | 0.2f;
}

/**
 * @brief Populates the analysis config with the JSON array holding the
 *        modules using the base structure found in /WebApp/src/data and
 *        derived structures found in /main/storage.h
 * 
 * @param modulesList - Reference to the JSON array holding the modules info
 * @param config - Pointer to the AnalysisConfig to update
 */
void Utils::populateModulesList(JsonArray& modulesList, AnalysisConfig* config)
{
  // Channel count
  int ch {0};

  for (auto module : modulesList)
  {
    if (ch >= (NUM_OUT_CH * 2)) break;

    // newModule type is ModuleConfig at the moment
    const auto Type = static_cast<ModuleType>(module["moduleType"]);
    auto newModule = createModule(Type);

    if (!newModule)
    {
      DEBUG_PRINTLN("WARNING: Memory could not be allocated for new module in populateModulesList");
      continue;
    }
    // Update base data
    auto* modulePtr = newModule.get();
    modulePtr->outputNumber = module["outputNumber"]; 
    modulePtr->freqLow = module["freqLow"]; 
    modulePtr->freqHigh = module["freqHigh"];
    modulePtr->minAmpNorm = module["minAmpNorm"];

    // Do the rest params under a function that takes in the type or use branching, for now just do this for MajorPeaks
    if (Type == MAJORPEAKS)
    {
      auto* majorPeaksConfig = static_cast<MajorPeaksConfig*>(newModule.get());

      majorPeaksConfig->frequencyMapping = static_cast<FrequencyMapping>(module["frequencyMapping"]);
      majorPeaksConfig->maxPeaks = module["maxPeaks"];
    }
    else if (Type == PERCUSSION)
    {
      auto* percussionConfig = static_cast<PercussionConfig*>(newModule.get());

      percussionConfig->fluxThresh = module["fluxThresh"];
      percussionConfig->energyThresh = module["energyThresh"];
      percussionConfig->entropyThresh = module["entropyThresh"];
      percussionConfig->waveType = static_cast<WaveType>(module["waveType"]);
    }
    // The old module memory will be deleted automatically since the ptr now has no references
    config->modules[ch] = std::move(newModule);
    ch++;
  }
}

/**
 * @brief Populates the referenced JSON object with global data from the
 *        AnalysisConfig using the JSON structure found in /WebApp/src/data.
 * 
 * @param global - Reference to the JSON object to be populated.
 * @param config - Pointer to the AnalysisConfig to pull data from.
 */
void Utils::packageGlobalSettings(JsonObject& global, AnalysisConfig* config)
{
  global["noiseFloor"] = config->noiseFloor;
  global["cfarRefCount"] = config->cfarRefCount;
  global["cfarGuardCount"] = config->cfarGuardCount;
  global["cfarBias"] = config->cfarBias;
  global["smoothingFactor"] = config->smoothingFactor;
}

/**
 * @brief Populates the references JSON array with module data from the
 *        AnalysisConfig using the JSON structure found in /WebApp/src/data.
 * 
 * @param modulesList - Reference to the modules array to populate.
 * @param config - Pointer to the AnalysisConfig to pull data from.
 */
void Utils::packageModulesList(JsonArray& modulesList, AnalysisConfig* config)
{
 for (auto i {0u}; i < NUM_OUT_CH * 2; i++)
 {
  if (config->modules[i] == nullptr)
    continue;

  // Allocate memory in the array
  JsonObject module = modulesList.add<JsonObject>();

  module["outputNumber"] = static_cast<int>(config->modules[i]->outputNumber);
  module["moduleType"] = static_cast<int>(config->modules[i]->moduleType);
  module["freqLow"] = static_cast<int>(config->modules[i]->freqLow);
  module["freqHigh"] = static_cast<int>(config->modules[i]->freqHigh);
  module["minAmpNorm"] = static_cast<float>(config->modules[i]->minAmpNorm);

  // Do the rest params under a function that takes in the type or use branching, for now just do this for MajorPeaks
  if (config->modules[i]->moduleType == MAJORPEAKS)
  {
    auto* modulePtr = static_cast<MajorPeaksConfig*>(config->modules[i].get());
    module["frequencyMapping"] = modulePtr->frequencyMapping;
    module["maxPeaks"] = modulePtr->maxPeaks;
  }
  else if (config->modules[i]->moduleType == PERCUSSION)
  {
    auto* modulePtr = static_cast<PercussionConfig*>(config->modules[i].get());
    module["fluxThresh"] = modulePtr->fluxThresh;
    module["energyThresh"] = modulePtr->energyThresh;
    module["entropyThresh"] = modulePtr->entropyThresh;
    module["waveType"] = modulePtr->waveType;
  }
 } 
}

/**
 * @brief Creates a unique pointer of the passed in module type using
 *        the factory method w/ a map.
 * 
 * @param Type - Type of module config to create.
 * @return ModulePtr - Unique pointer of the passed in module type.
 */
inline ModulePtr Utils::createModule(const ModuleType Type)
{
  // Used to create module configs
  static const ModuleFactory Map =
  {
    // TODO: add more modules types as we create structs for them
    { MAJORPEAKS, []() { return std::make_unique<MajorPeaksConfig>(0, 0, 0, 10000.0, NONE, 1); } },
    { PERCUSSION, []() { return std::make_unique<PercussionConfig>(0, 0, 0, 10000.0, 0.0, 0.0, 0.0, SINE); } },
  };
  // If the module type is defined, we return the second element since the Map carries a pair:
  // Ex: ( ModuleType, unique_ptr for module )
  auto itr = Map.find(Type);
  return (itr == Map.end() ? nullptr : itr->second());
}
