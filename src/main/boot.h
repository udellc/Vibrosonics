/***************************************************************
 * FILE: boot.h
 * 
 * DATE: 11/17/2025
 * 
 * DESCRIPTION: Initializes the ESP32 components before running
 * the web app and audio analysis
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#ifndef BOOT_H
#define BOOT_H

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// SD card pinouts
// #define SCK 5
// #define MISO 19
// #define MOSI 18
// #define CS 21

namespace InternalNetwork
{
  bool init();
}

namespace WebServer
{
  bool init();
}

namespace FileSys
{
  bool init();
}

#endif
