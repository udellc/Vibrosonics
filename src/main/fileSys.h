/***************************************************************
 * FILE: fileSys.h
 * 
 * DATE: 11/18/2025
 * 
 * DESCRIPTION: This namespace contains functions to initialize
 * and manage files and non-volatile memory for the SD card 
 * connected on the ESP32.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#ifndef FILE_SYS_H
#define FILE_SYS_H

#include <SPI.h>
#include <SD.h>

// SD card pins
#define SCK_PIN 5
#define MISO_PIN 19
#define MOSI_PIN 18
#define CS_PIN 16 

namespace FileSys
{
  bool init();
}

#endif
