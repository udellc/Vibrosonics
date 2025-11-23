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

// Error HTML file that gets sent if the web app is not found in SD memory
static const char *noWebAppContent PROGMEM = R"(
<!DOCTYPE html>
<html>
<head>
  <title>Server-Sent Events</title>
</head>
<body>
  <h1>Web app not on SD card!</h1>
</body>
</html>
)";

static AsyncWebServer server(80);

// TODO: add header comment
bool WebServer::init()
{
  bool success = true;

  Serial.println("Starting web server...");

  // TODO: Add Vibrosonics APIS
  // TODO: Add WiFi/server monitoring APIs if needed

  success &= FileSys::exists("/index.html");

  // Lamnda for initial request
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
  {
    // TODO: find out how to add web app files to SD card
    // NOTE: the current SD card has the AudioLux web files loaded onto it, we need to remove them and replace them with
    //       the Vibrosonics web app files
    if (!FileSys::exists("/index.html"))
    {
      req->send(HTTP_OK, "text/html", noWebAppContent);
    }
    else
    {
      //! NOTE: This should be the only time SD is referenced outside of the FileSys namespace
      req->send(SD, "/index.html", getContentType("/index.html"));
    }
  });
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
