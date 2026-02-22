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

// TODO: add header comment
void Utils::populateGlobalSettings(JsonObject& global, AnalysisConfig* config)
{
  config->noiseFloor = global["noiseFloor"] | 280.0f;
  config->cfarRefCount = global["cfarRefCount"] | 6;
  config->cfarGuardCount = global["cfarGuardCount"] | 1;
  config->cfarBias = global["cfarBias"] | 1.2f;
  config->smoothingFactor = global["smoothingFactor"] | 0.2f;
}

// TODO: add header comment and implement
void Utils::populateModulesList(JsonArray& modulesList, AnalysisConfig* config)
{
  int ch {0};

  for (auto module : modulesList)
  {
    if (ch >= NUM_OUT_CH) break;

    const auto Type = static_cast<ModuleType>(module["moduleType"]);
    auto newModule = createModule(Type);

    if (!newModule)
    {
      DEBUG_PRINTLN("WARNING: Memory could not be allocated for new module in populateModulesList");
      continue;
    }
    auto* modulePtr = newModule.get();
    modulePtr->freqLow = module["freqLow"]; 
    modulePtr->freqHigh = module["freqHigh"];
    modulePtr->frequencyMapping = static_cast<FrequencyMapping>(module["frequencyMapping"]);
    modulePtr->minAmpNorm = module["minAmpNorm"];

    // Do the rest params under a function that takes in the type or use branching, for now just do this for MajorPeaks
    if (Type == MAJORPEAKS)
    {
      auto* majorPeaksConfig = static_cast<MajorPeaksConfig*>(newModule.get());

      majorPeaksConfig->maxPeaks = module["maxPeaks"];
    }
    config->modules[ch] = std::move(newModule);
    ch++;
  }
}

// TODO: add header comment
void Utils::packageGlobalSettings(JsonObject& global, AnalysisConfig* config)
{
  global["noiseFloor"] = config->noiseFloor;
  global["cfarRefCount"] = config->cfarRefCount;
  global["cfarGuardCount"] = config->cfarGuardCount;
  global["cfarBias"] = config->cfarBias;
  global["smoothingFactor"] = config->smoothingFactor;
}

// TODO: add header comment
void Utils::packageModulesList(JsonArray& modulesList, AnalysisConfig* config)
{
 for (auto i {0u}; i < NUM_OUT_CH; i++)
 {
  if (config->modules[i] == nullptr) continue;

  // Allocate memory in the list
  JsonObject module = modulesList.add<JsonObject>();

  module["moduleType"] = static_cast<int>(config->modules[i]->moduleType);
  module["freqLow"] = static_cast<int>(config->modules[i]->freqLow);
  module["freqHigh"] = static_cast<int>(config->modules[i]->freqHigh);
  module["frequencyMapping"] = static_cast<int>(config->modules[i]->frequencyMapping);
  module["minAmpNorm"] = static_cast<float>(config->modules[i]->minAmpNorm);

  // Do the rest params under a function that takes in the type or use branching, for now just do this for MajorPeaks
  if (config->modules[i]->moduleType == MAJORPEAKS)
  {
    auto* modulePtr = static_cast<MajorPeaksConfig*>(config->modules[i].get());

    module["maxPeaks"] = modulePtr->maxPeaks;
  }
 } 
}

// TODO: add header comment
inline ModulePtr Utils::createModule(const ModuleType Type)
{
  // Used to create module configs
  // TODO: add more modules types as we create structs for them
  static const ModuleFactory Map =
  {
    { MAJORPEAKS, []() { return std::make_unique<MajorPeaksConfig>(0, 0, NONE, 10000.0, 1); } }
  };
  auto itr = Map.find(Type);
  return (itr == Map.end() ? nullptr : itr->second());
}
