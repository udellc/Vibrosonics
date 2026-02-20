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
#include "storage.h"
#include <Arduino.h>
#include <memory>

HapticSettings::HapticSettings() :
  curConfig{ nullptr }
{
  this->curConfig = std::make_shared<AnalysisConfig>();
}

// TODO: implement
bool HapticSettings::loadConfig()
{
  // Get and parse JSON file with the settings into variable curConfig
  // If no error,
    // load the settings
  // if there is an error,
    // load a preset thats in storage.h
    // NOTE: we could have a "safe" preset in the code just in case the SD card fails
  return false;
}
