/***************************************************************
 * FILE: hapticSettings.cpp
 * 
 * DATE: 02/16/2026
 * 
 * DESCRIPTION: HapticSettings singleton class implementation
 * file.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#include "hapticSettings.h"
#include "config.h"
#include "fileSys.h"
#include "storage.h"
#include "utils.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <memory>

#define MAIN_ANALYSIS_PATH "/data/mainConfig.json"

HapticSettings::HapticSettings()
    : curConfig { nullptr }
{
  this->curConfig = std::make_shared<AnalysisConfig>();
}

// TODO: implement
bool HapticSettings::loadConfig()
{
  JsonDocument doc;
  bool usingSavedConfig {false};
  const bool HasConfig = FileSys::exists(MAIN_ANALYSIS_PATH);

  if (HasConfig)
  {
    File configFile = FileSys::getFile(MAIN_ANALYSIS_PATH);
    const auto Error = deserializeJson(doc, configFile);

    if (!Error)
    {
      JsonObject globalSettings = doc["global"].as<JsonObject>();
      JsonArray modulesList = doc["modules"].as<JsonArray>();

      Utils::populateGlobalSettings(globalSettings, this->curConfig.get());
      Utils::populateModulesList(modulesList, this->curConfig.get());

      usingSavedConfig = true;
    }
    else
      DEBUG_PRINTLN("WARNING: Saved analysis config could not be serialized");
  }
  if (!usingSavedConfig)
  {
    DEBUG_PRINTLN("WARNING: Using preset for analysis config");

    this->curConfig = std::make_shared<AnalysisConfig>(
      AnalysisConfig(
        280, 6, 1, 1.4, 0.2,
        {
          std::make_unique<MajorPeaksConfig>(400, 1000, OCTAVE, 10000.0, 1),
          std::make_unique<MajorPeaksConfig>(1000, 3600, OCTAVE, 10000.0, 1)
        }
      )
    );
  }
  return usingSavedConfig;
}
