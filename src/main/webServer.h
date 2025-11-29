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
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "config.h"

namespace WebServer
{
  //! Initializes the web server before starting it
  bool init();

  //! Initializes the web server assuming the web app is setup correctly on the SD card
  inline void setupWebApp();

  //!
  void sendScannedNetworks(AsyncWebServerRequest *req);

  //! 
  void sendNetworkConnectResponse(AsyncWebServerRequest *req);

  //! Helper function for returning the content type
  String getContentType(const String &Path);

  #ifdef UPLOAD_MODE
    //! Initializes the web server assuming the web app in upload files mode 
    inline void setupUploadMode();

    //! Handler for the initial request to the host
    void sendUploadPage(AsyncWebServerRequest *req);

    //! Handler for writing a file into the SD card
    void handleUpload(AsyncWebServerRequest *req, String filename, size_t index, uint8_t *data, size_t len, bool final);

    //! Prints the contents of the root directory in the SD card
    void printFiles(AsyncWebServerRequest *req);

    void clearSD(AsyncWebServerRequest *req);
  #endif
}

#endif
