/***************************************************************
 * FILE: utils.cpp
 * 
 * DATE: 2/19/2026
 * 
 * DESCRIPTION: Implementation file for the Utils namespace.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#include "utils.h"
#include "config.h"

void Utils::populateGlobalSettings(JsonObject& global, AnalysisConfig* config)
{
  config->noiseFloor = global["noiseFloor"]["value"];
  config->cfarRefCount = global["cfarRefCount"]["value"];
  config->cfarGuardCount = global["cfarGuardCount"]["value"];
  config->cfarBias = global["cfarBias"]["value"];
  config->smoothingFactor = global["smoothingFactor"]["value"];
}

void Utils::populateModulesList(JsonArray& modulesList, AnalysisConfig* config)
{
  ModuleConfig** modules = config->modules;

  for (auto module : modulesList)
  {
    
  }
}
