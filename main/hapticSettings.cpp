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

HapticSettings::HapticSettings()
{
  this->curConfig = std::make_shared<AnalysisConfig>();
}

// TODO: implement
bool HapticSettings::loadConfig()
{
  return false;
}

// TODO: implement
bool HapticSettings::updateConfig(AnalysisConfig& other)
{
  return false;
}
