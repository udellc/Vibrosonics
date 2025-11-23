/***************************************************************
 * FILE: fileSys.cpp
 * 
 * DATE: 11/18/2025
 * 
 * DESCRIPTION: The implemenation file for the FileSys namespace.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#include "fileSys.h"

/**
 * @brief Initializes the SD card file system
 * 
 * @return Bool indicating if the SD card successfully initialized 
 */
bool FileSys::init()
{
  bool success = true;
  Serial.println("File system initializing...");

  if (!SD.begin(CS_PIN))
  {
    Serial.println("SD File system failed to initialize.");
    success = false;
  }
  Serial.println("SD File system successfully initialized.");

  return success;
}

/**
 * @brief Writes content to the given file in the SD card in truncate mode
 * 
 * @param Path - File we want to write to
 * 
 * @param Data - Content we want to write to the file
 * 
 * @return Bool inidicating of the file can be opened from the SD card
 * 
 * NOTE: If the file does not exist in the SD card, it will be created
 */
bool FileSys::writeFile(const String &Path, const String &Data)
{
  File file = SD.open(Path, FILE_WRITE);

  if (!file)
  {
    return false;
  }
  (void) file.print(Data);
  file.close();

  return true;
}

/**
 * @brief Writes content to the given file in the SD card in appends mode
 * 
 * @param Path - File we want to write to
 * 
 * @param Data - Content we want to write to the file
 * 
 * @return Bool inidicating of the file can be opened from the SD card
 * 
 * NOTE: If the file does not exist in the SD card, it will be created
 */
bool FileSys::appendFile(const String &Path, const String &Data)
{
  File file = SD.open(Path, FILE_APPEND);

  if (!file)
  {
    return false;
  }
  (void) file.print(Data);
  file.close();

  return true;
}

/**
 * @brief Reads the contents of the given file from the SD card and returns
 *        the contents
 * 
 * @param Path - File we want ot read from
 * 
 * @return String representing the contents of the file
 */
String FileSys::readFile(const String &Path)
{
  File file = SD.open(Path, FILE_READ);

  if (!file)
  {
    return "";
  }
  const String Data = file.readString();
  file.close();

  return Data;
}

#ifdef UPLOAD_MODE

/**
 * @brief Returns a JSON style array as a string for the files within the given directory path
 * 
 * @param Dir - Directory path to list files from
 * 
 * @param Print - Bool indicating if the files should be printed to the Serial monitor
 * 
 * @return JSON style array as a string, representing all files in directory of the SD card 
 * 
 * NOTE: This function should only exist in dev environment to view the SD card files
 */
String FileSys::listFiles(const String &Dir, const bool Print)
{
  File root = SD.open(Dir);

  // Nothing found in the directory, return empty array
  if (!root)
  {
    return "[]";
  }
  String files = "[";
  File file = root.openNextFile();
  
  while (file)
  {
    files += "/" + String(file.name()) + ",";

    if (Print)
    {
      const char *Type = (file.isDirectory()) ?  "Type: Directory\t" : "Type: File\t";
      Serial.print(Type);
      Serial.println(file.name());
    }
    file = root.openNextFile();
  }
  // Remove last comma
  files.remove( (files.length() - 1) );
  files += "]";

  return files;
}

#endif
