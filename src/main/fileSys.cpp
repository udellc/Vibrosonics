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
