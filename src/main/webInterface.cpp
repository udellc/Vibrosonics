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

#include "webInterface.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <set>
#include "config.h"
#include "fileSys.h"
#include "networking.h"

// HTTP defines
constexpr int HTTP_OK = 200;
constexpr int HTTP_ACCEPTED = 202;
constexpr int HTTP_BAD_REQUEST = 400;
constexpr int HTTP_NOT_FOUND = 404;
constexpr int HTTP_UNPROCESSABLE = 422;
constexpr int HTTP_INTERNAL_ERROR = 500;
constexpr int HTTP_UNAVAILABLE = 503;

// Content type defines
constexpr char TEXT_PLAIN[] = "text/plain";
constexpr char TEXT_HTML[] = "text/html";
constexpr char TEXT_CSS[] = "text/css";
constexpr char APP_JAVASCRIPT[] = "application/javascript";
constexpr char IMAGE_PNG[] = "image/png";
constexpr char IMAGE_JPEG[] = "image/jpeg";
constexpr char IMAGE_X_ICON[] = "image/x-icon";
constexpr char APP_JSON[] = "application/json";

// Internal web server functions
static String getContentType(const String &Path);
static bool parsePayload(JsonDocument &output);

// Web server global stuff
#define SERVER_PORT 80u

static WebServer server(SERVER_PORT);

// File buffer object for uploading files from machine to ESP32 
File _uploadFile;

/**
 * @brief Initializes the web server with file uploading capabilities,
 *        depending on the config file
 * 
 * @return Bool inidicating if the web server is live
 */
bool WebInterface::init()
{
  bool success = true;

  Serial.println("Starting web server...");

  success &= FileSys::exists("/index.html");

  #ifdef DEV_MODE
    setupUploadMode();
  #endif
    setupServer();

  if (!success) Serial.println("/index.html not found. Missing web app files.");
  
  server.begin();
  Serial.println("Web server started.");

  return success;
}

// TODO: add header comment
void WebInterface::run()
{
  server.handleClient();
}

// TODO: add header comment
inline void WebInterface::setupServer()
{
  server.on("/", HTTP_GET, sendWebApp);

  // Network APIs
  server.on("/network/scanNetworks", HTTP_GET, onScanNetworks);
  server.on("/network/connect", HTTP_POST, onConnectToNetwork);
  server.on("/network/getSsid", HTTP_GET, []()
  {
    server.send(HTTP_OK, TEXT_PLAIN, Networking::getNetworkSsid());
  });
  server.serveStatic("/", SD, "/");
  server.onNotFound(onNotFoundHandler);
}

// TODO: add header comment
void WebInterface::sendWebApp()
{
  File entryFile = FileSys::getFile("/index.html");

  if (entryFile)
  {
    server.streamFile(entryFile, TEXT_HTML);
    entryFile.close();
  }
  else server.send(HTTP_NOT_FOUND, TEXT_PLAIN, "File not found");
}

// TODO: add header comment
void WebInterface::onNotFoundHandler()
{
  const String Path = server.uri();

  if (!FileSys::exists(Path)) server.send(HTTP_NOT_FOUND, TEXT_PLAIN, "404: Not found");
  
  else
  {
    Serial.println("DEBUG: im here in onFoundHandler");
    File file = FileSys::getFile(Path);

    if (file)
    {
      Serial.printf("DEBUG: file name %s\n", file.path());
      server.streamFile(file, getContentType(Path));
      file.close();
    }
    else server.send(HTTP_NOT_FOUND, TEXT_PLAIN, "404: Not found");
  }
}

void WebInterface::onScanNetworks()
{
  String json;
  JsonDocument doc;
  std::set<String> networks;

  Networking::scanAvailableNetworks(networks);

  // Convert networks into json for frontend to parse
  JsonArray jsonNetworks = doc["ssid"].to<JsonArray>();

  for (const auto &ssid : networks) jsonNetworks.add(ssid);
  
  serializeJson(doc, json);
  server.send(HTTP_OK, APP_JSON, json);
}

void WebInterface::onConnectToNetwork()
{
  JsonDocument payload;
  int resStatus = HTTP_UNPROCESSABLE;
  bool hasConnected = false;

  if (parsePayload(payload))
  {
    const String SelectedNetwork = payload["selectedNetwork"] | "";
    const String Password = payload["password"] | "";
    hasConnected = Networking::connectToNetwork(SelectedNetwork, Password);
  }
  if (hasConnected) resStatus = HTTP_ACCEPTED;

  server.send(resStatus);
}

/**
 * @brief Helper function for getting the content type of a file, given the file name
 * 
 * @param Path - Reference to the name of the file we want to get the type for
 * 
 * @return String for the content type of the file 
 */
static String getContentType(const String &Path)
{
  if (Path.endsWith(".html")) return TEXT_PLAIN;
  if (Path.endsWith(".css"))  return TEXT_CSS;
  if (Path.endsWith(".js"))   return APP_JAVASCRIPT;
  if (Path.endsWith(".png"))  return IMAGE_PNG;
  if (Path.endsWith(".jpg"))  return IMAGE_JPEG;
  if (Path.endsWith(".ico"))  return IMAGE_X_ICON;
  if (Path.endsWith(".json")) return APP_JSON;
  
  return TEXT_PLAIN;
}

// TODO: add header comment
static bool parsePayload(JsonDocument &output)
{
  // "plain" is used to specify the reuqest body holding the data
  if (!server.hasArg("plain")) return false;

  String body = server.arg("plain");
  const auto ParsingError = deserializeJson(output, body);

  if (ParsingError)
  {
    Serial.println("DEBUG: parsing error in parsePayload");

    return false;
  }
  return true;
}

#ifdef DEV_MODE

static const char *uploadForm PROGMEM = R"(
<!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Upload Mode</title>
  </head>
  <body>
    <h1>In Upload Mode</h1>
    <h3>Upload a File</h3>
    <form method='POST' action='/dev/upload' enctype='multipart/form-data'>
      <label for='directoryName'>Target Directory</label>
      <input type='text' name='directoryName' value='/'>
      
      <label for='fileName'>Target File</label>
      <input type='file' name='fileName'>
      
      <input type='submit' value='Upload'>
    </form>
    <form method='POST' action='/dev/printFiles'>
      <label for='printFiles'>See root directory content</label>
      <input type='submit' value='Print Files'>
    </form>

    <form method='POST' action='/dev/clearSd'>
      <label for='clearSd'>Clear Content</label>
      <input type='submit' value='Clear SD'>
    </form>
  </body>
  </html>
)";

/**
 * @brief Adds endpoint API handlers for dev mode
 * 
 */
inline void WebInterface::setupUploadMode()
{
  server.on("/dev", HTTP_GET, []()
  {
    server.send(HTTP_OK, TEXT_HTML, uploadForm);
  });
  server.on("/dev/upload", HTTP_POST, uploadFile);
  server.on("/dev/printFiles", HTTP_POST, printFiles);
  server.on("/dev/clearSd", HTTP_POST, clearSd);
}

// TODO: add header comment
void WebInterface::uploadFile()
{
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START)
  {
    String dir = server.arg("directoryName");

    if (!dir.startsWith("/")) dir = "/" + dir;
    if (!dir.endsWith("/")) dir += "/";

    String filename = upload.filename;
    String fullPath = dir + filename;

    Serial.printf("Uploading to: %s\n", fullPath.c_str());
    _uploadFile = FileSys::getFile(fullPath, FILE_WRITE);
  } 
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (_uploadFile) _uploadFile.write(upload.buf, upload.currentSize);
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (_uploadFile)
    {
      _uploadFile.close();
      Serial.printf("Upload Success: %u bytes\n", upload.totalSize);
    }
  }
}

// TODO: add header comment
void WebInterface::printFiles()
{
  Serial.println("Printing files...");
  File root = FileSys::getFile();

  if (root)
  {
    FileSys::traverseFiles(root, FileSys::printFile);

    server.send(HTTP_OK, "text/plain", "Printed files to serial monitor");
    root.close();
  }
  else
  {
    server.send(HTTP_BAD_REQUEST, "text/plain", "Invalid root provided");
  }
}

void WebInterface::clearSd()
{
  Serial.println("Clearing SD memory...");
  File root = FileSys::getFile();

  if (root)
  {
    FileSys::traverseFiles(root, FileSys::removeFile);

    root.close();
    server.send(HTTP_OK, "text/plain", "SD File System Cleared");
  }
  else
  {
    server.send(HTTP_BAD_REQUEST, "text/plain", "Invalid root provided");
  }
}

#endif // DEV_MODE
