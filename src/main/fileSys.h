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
#include "config.h"

// SD card pins
#define SCK_PIN 5
#define MISO_PIN 19
#define MOSI_PIN 18
#define CS_PIN 16 

namespace FileSys
{
  //! Inits the connected SD card
  bool init();

  //! Checks for existance if a file
  inline bool exists(const String &Path) { return ( SD.exists(Path) ); }

  //! Removes a file from the SD card
  inline bool remove(const String &Path) { return ( SD.remove(Path) ); }

  //! Writes a file to the SD card, truncating the previous data
  bool writeFile(const String &Path, const String &Data);

  //! Appends data to an existing file in the SD card
  bool appendFile(const String &Path, const String &Data);

  //! Reads the contents of the given file and returns it as a string 
  String readFile(const String &Path);

  #ifdef UPLOAD_MODE
    //! Returns a JSON array as a string for the files in the given directory
    String listFiles(const String &Dir, const bool Print);

  #endif
}

#endif
