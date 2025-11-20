/***************************************************************
 * FILE: webServer.cpp
 * 
 * DATE: 11/18/2025
 * 
 * DESCRIPTION: The implementation file for the WebServer
 * namespace.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#include "webServer.h"

AsyncWebServer server(80);

// TODO: add header comment
bool WebServer::init()
{
  return false;
}

// TODO: add header comment
String WebServer::getContentType(const String &Path) {
  if (Path.endsWith(".html")) return "text/html";
  if (Path.endsWith(".css"))  return "text/css";
  if (Path.endsWith(".js"))   return "application/javascript";
  if (Path.endsWith(".png"))  return "image/png";
  if (Path.endsWith(".jpg"))  return "image/jpeg";
  if (Path.endsWith(".ico"))  return "image/x-icon";
  if (Path.endsWith(".json")) return "application/json";
  return "text/plain";
}
