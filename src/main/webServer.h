/***************************************************************
 * FILE: webServer.h
 * 
 * DATE: 11/18/2025
 * 
 * DESCRIPTION: This namespace contains functions to initialize
 * and manage the web server deplyed on the ESP32.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <string.h>

namespace WebServer
{
  //! Initializes the web server before starting it
  bool init();

  //! Initializes the web server assuming the web app is setup correctly on the SD card
  inline void setupWebApp();

  #ifdef UPLOAD_MODE
    //! Initializes the web server assuming the web app is not setup correctly on the SD card  
    inline void setupUploadMode();
  #endif

  //! Helper function for returning the content type
  String getContentType(const String &Path);
}

#endif
