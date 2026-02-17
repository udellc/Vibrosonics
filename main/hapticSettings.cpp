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
#include <Arduino.h>

HapticSettings::HapticSettings() :
  counter{ 0 }
{
  DEBUG_PRINTLN("DEBUG: initialized HapticSettings class");
}
