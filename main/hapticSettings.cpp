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

HapticSettings::HapticSettings() :
  curConfig{ nullptr },
  _isDirty{ false }
{}

/**
 * @brief Populates the analysis configurations using the data found in the
 *        SD card, falling back onto an internal preset of the data file is not
 *        found.
 * 
 * @return Bool indicating if the loaded configuration was found from SD card.
 *         True if found on SD card.
 *         False if using preset.
 */
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
      this->curConfig = std::make_shared<AnalysisConfig>();
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
    DEBUG_PRINTLN("WARNING: Using internal preset for analysis config");

    this->curConfig = std::make_shared<AnalysisConfig>();

    this->curConfig->noiseFloor = 280;
    this->curConfig->cfarRefCount = 6;
    this->curConfig->cfarGuardCount = 1;
    this->curConfig->cfarBias = 1.4;
    this->curConfig->smoothingFactor = 0.2;

    this->curConfig->modules[0] =
      std::make_unique<PercussionConfig>(1, 1800, 4000, 10000000.0, 0.5, 100000000.0, 0.78, TRIANGLE);
    this->curConfig->modules[1] =
      std::make_unique<PercussionConfig>(0, 1800, 4000, 10000000.0, 0.5, 100000000.0, 0.78, TRIANGLE);
    // this->curConfig->modules[2] =
    //   std::make_unique<MajorPeaksConfig>(1, 1000, 3600, 10000.0, OCTAVE, 1);
    // this->curConfig->modules[3] =
    //   std::make_unique<MajorPeaksConfig>(0, 400, 1000, 10000.0, OCTAVE, 1);
  }
  this->setIsDirty(true);

  return usingSavedConfig;
}
