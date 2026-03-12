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

  //! Looks for index.html file on the SD card and send it
  void sendWebApp();

  //! Handler for invalid URI requests
  void onNotFoundHandler();

  //! Handler for sending ESP32 scanned WiFi networks
  void onScanNetworks();

  //! Handler for connncting to user selected network
  void onConnectToNetwork();

  //! Handler for getting the current analysis config settings
  void sendAnalysisConfig();

  //! Handler for updating the HapticSettings config
  void onSubmitConfig();

  //! Handler for real-time updates
  void onEditSetting();

  #ifdef DEV_MODE_EN
    //! Initializes the web server assuming the web app in upload files mode 
    inline void setupUploadMode();

    //! Uploads a file to the SD card
    void uploadFile();

    //! Prints all files on the SD card on the serial monitor
    void printFiles();

    //! Removes all files from the SD card
    void clearSd();

    //! Prints the heap memory stats to the serial monitor    
    void getMemory();
  #endif // DEV_MODE_EN
}

#endif // WEB_INTERFACE_H
