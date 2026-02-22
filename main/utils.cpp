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
  // ModuleConfig** modules = config->modules.get();

  for (const auto module : modulesList)
  {
    const auto Type = module["type"];
    // DEBUG_PRINTF("DEBUG: module type: %s\n", Type);

    // TODO: use createModule then add params then add to config argument
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
