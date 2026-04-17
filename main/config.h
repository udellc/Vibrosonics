/***************************************************************
 * FILE: config.h
 * 
 * DATE: 11/22/2025
 * 
 * DESCRIPTION: File used for the ESP32 boot up mode.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

// Allow API endpoints to interact with the file system
#define DEV_MODE_EN

// Allow debug statements to be printed to the Serial Monitor
// Usage: true = enabled, false = disabled
#define DEBUG_EN true

#if DEBUG_EN
  #define DEBUG_BEGIN(baudRate) Serial.begin(baudRate); Serial.println("DEBUG: Debugging Enabled")
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
  #define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
  #define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
  // If DEBUG_EN not defined,  macros expand to nothing, and compiler optimizes them away
  #define DEBUG_BEGIN(baudRate)
  #define DEBUG_PRINTF(...)
  #define DEBUG_PRINT(...)
  #define DEBUG_PRINTLN(...)
#endif

#endif
