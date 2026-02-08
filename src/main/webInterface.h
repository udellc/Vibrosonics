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

#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include "config.h"

namespace WebInterface
{
  //! Initializes the web server before starting it
  bool init();

  //! Function call for the server to handle requests via polling
  void run();

  //! Adds API endpoints for the web server
  inline void setupServer();

  // 
  void sendWebApp();

  void onNotFoundHandler();

  void onScanNetworks();

  void onConnectToNetwork();

  #ifdef DEV_MODE_EN
    //! Initializes the web server assuming the web app in upload files mode 
    inline void setupUploadMode();

    //! Uploads a file to the SD card
    void uploadFile();

    //! Prints all files on the SD card on the serial monitor
    void printFiles();

    //! Removes all files from the SD card
    void clearSd();

  #endif  // DEV_MODE_EN
}

#endif // WEB_INTERFACE_H
