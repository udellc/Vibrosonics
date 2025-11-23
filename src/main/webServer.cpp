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
#include "fileSys.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// HTTP defines
constexpr int HTTP_OK = 200;
constexpr int HTTP_ACCEPTED = 202;
constexpr int HTTP_BAD_REQUEST = 400;
constexpr int HTTP_UNAUTHORIZED = 401;
constexpr int HTTP_METHOD_NOT_ALLOWED = 405;
constexpr int HTTP_UNPROCESSABLE = 422;
constexpr int HTTP_INTERNAL_ERROR = 500;
constexpr int HTTP_UNAVAILABLE = 503;

static AsyncWebServer server(80);

// TODO: add header comment
bool WebServer::init()
{
  bool success = true;

  Serial.println("Starting web server...");

  // TODO: Add Vibrosonics APIS
  // TODO: Add WiFi/server monitoring APIs if needed
  success &= FileSys::exists("/index.html");

  #ifdef UPLOAD_MODE
    setupUploadMode();
  #else
    setupWebApp();
  #endif

  server.begin();
  Serial.println("Web server started.");

  return success;
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

inline void WebServer::setupWebApp()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
  {
    //! NOTE: This should be the only time SD is referenced outside of the FileSys namespace
    req->send(SD, "/index.html", getContentType("/index.html"));
  });
}

#ifdef UPLOAD_MODE
inline void WebServer::setupUploadMode()
{
  // Sends the form
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
  {
    req->send(HTTP_OK, "text/html",
      "<!DOCTYPE html>
      <html>
      <head>
        <title>Upload Mode</title>
      </head>
      <body>
        <h1>In Upload Mode</h1>
        <h3>Upload a File</h3>
        <form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='data_file' webkitdirectory multiple>
          <input type='submit' value='Upload'>
        </form>
      </body>
      </html>"
    );
  });

  // TODO: add an upload API using the FileSys::writeFile() to see if it works
  // server.on("/upload", HTTP_POST, );

  // TODO: add a make directory API for the assets directory produced by Vite
}
#endif
