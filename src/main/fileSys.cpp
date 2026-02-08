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
  DEBUG_PRINTLN("DEBUG: File system initializing...");

  if (!SD.begin(CS_PIN))
  {
    DEBUG_PRINTLN("DEBUG: SD File system failed to initialize.");
    success = false;
  }
  else
  {
    DEBUG_PRINTLN("DEBUG: SD File system successfully initialized.");
  }
  return success;
}

/**
 * @brief Writes content to the given file in the SD card in truncate mode
 * 
 * @param Path - File we want to write to
 * @param Data - Content we want to write to the file
 * 
 * @return Bool inidicating of the file can be opened from the SD card
 * 
 * NOTE: If the file does not exist in the SD card, it will be created
 */
bool FileSys::writeFile(const String &Path, const String &Data)
{
  File file = SD.open(Path, FILE_WRITE, true);

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
 * @param Data - Content we want to write to the file
 * 
 * @return Bool inidicating of the file can be opened from the SD card
 * 
 * NOTE: If the file does not exist in the SD card, it will be created
 */
bool FileSys::appendFile(const String &Path, const String &Data)
{
  File file = SD.open(Path, FILE_APPEND, true);

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

#ifdef DEV_MODE_EN

/**
 * @brief Helper function to access all files in the SD card and apply a callback to each file.
 * 
 * @param start - File or directory to start from
 * @param callback - Function to be applied for every file it sees
 */
void FileSys::traverseFiles(File start, FSCallback callback)
{
  while (1)
  {
    File next = start.openNextFile();

    if (!next)
    {
      break;
    }
    if (next.isDirectory())
    {
      traverseFiles(next, callback);
    }
    else
    {
      callback(next);
    }
    next.close();
  }
}

/**
 * @brief Prints the file to the Serial monitor.
 * 
 * @param file - File we want to be printed
 */
void FileSys::printFile(File &file)
{
  const String FileType = (file.isDirectory()) ? "Directory" : "File";
  DEBUG_PRINTF("DEBUG: Type: %s\tName: %s\tPath: %s\n", FileType, file.name(), file.path());
}

/**
 * @brief Removes the file form the SD card
 * 
 * @param file - File we want to remove
 */
void FileSys::removeFile(File &file)
{
  (void) remove(file.path()); 
}

#endif
