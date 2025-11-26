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

// A generic alias for the traverseFiles function, this is called for each file in the SD card
typedef void (*FSCallback)(File& file);

namespace FileSys
{
  //! Inits the connected SD card
  bool init();

  //! Getter for a file given a path
  inline File getFile(const String &Path = "/", const char *mode = FILE_READ) { return SD.open(Path, mode); }

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
    //! Applies the callback function for every file in the SD card
    void traverseFiles(File start, FSCallback callback);

    //! Prints the type, name, and path of the given file
    void printFile(File &file);

  #endif
}

#endif
