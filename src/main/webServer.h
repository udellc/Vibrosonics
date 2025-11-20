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

#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

namespace WebServer
{
  bool init();
  String getContentType(const String &Path);
}

#endif
