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

#ifndef FILESYS_H
#define FILESYS_H

// SD card pinouts
// #define SCK 5
// #define MISO 19
// #define MOSI 18
// #define CS 21

namespace FileSys
{
  bool init();
}

#endif
