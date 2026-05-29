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
#include <memory>
#include "config.h"
#include "fileSys.h"
#include "hapticSettings.h"
#include "networking.h"
#include "storage.h"
#include "utils.h"

// HTTP defines
constexpr int HTTP_OK = 200;
constexpr int HTTP_ACCEPTED = 202;
constexpr int HTTP_BAD_REQUEST = 400;
constexpr int HTTP_NOT_FOUND = 404;
constexpr int HTTP_UNPROCESSABLE = 422;
constexpr int HTTP_TOO_MANY_REQUESTS = 429;
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

// Web server global stuff
#define SERVER_PORT 80u

static WebServer server(SERVER_PORT);

#ifdef DEV_MODE_EN
// File buffer object for uploading files from machine to ESP32 
File _uploadFile;
#endif

// Internal web server functions
static String getContentType(const String &Path);
static bool parsePayload(JsonDocument &output);
inline void limitReqRate(const unsigned long Time_ms);

/**
 * @brief Function that bypasses CORS restrictions when DEV_MODE_EN is enabled.
 * 
 * @param Code - HTTP status code to send
 * @param ContentType - Content type of the data to send
 * @param Content - Content to send. Should be parsed into a JSON string by the caller
 */
void inline send(const int Code, const char* ContentType = NULL, const String& Content = String(""))
{
#ifdef DEV_MODE_EN
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "*");
  server.sendHeader("Access-Control-Allow-Headers", "*");
#endif
  server.send(Code, ContentType, Content);
}

/**
 * @brief Initializes the web server with file uploading capabilities,
 *        depending on the config file.
 * 
 * @return Bool inidicating if the web server is live.
 * 
 */
bool WebInterface::init()
{
  bool success = true;

  DEBUG_PRINTLN("DEBUG: Starting web server...");

  success &= FileSys::exists("/index.html");

  #ifdef DEV_MODE_EN
    setupUploadMode();
  #endif
    setupServer();

  if (!success)
    DEBUG_PRINTLN("FATAL: /index.html not found. Missing web app files.");
  
  DEBUG_PRINTLN("DEBUG: Web server started.");

  return success;
}

/**
 * @brief Start the web server
 */
void WebInterface::start()
{
  server.begin();
}

/**
 * @brief Function call for running the web server via polling method.
 */
void WebInterface::run()
{
  server.handleClient();
}

/**
 * @brief Adds web server API endpoints and sets up WebServer settings.
 */
inline void WebInterface::setupServer()
{
  server.on("/", HTTP_GET, sendWebApp);

  // Network APIs
  server.on("/network/scanNetworks", HTTP_GET, onScanNetworks);
  server.on("/network/connect", HTTP_POST, onConnectToNetwork);
  server.on("/network/getInfo", HTTP_GET, onGetNetworkInfo);
  server.on("/network/saveAPSettings", HTTP_PATCH, onSaveAPSettings);
  server.on("/network/forgetWifi", HTTP_PATCH, onForgetWiFi);
  server.on("/network/resetSettings", HTTP_PATCH, onResetNetworkSettings);

  // Haptic settings APIs
  server.on("/analysis/getSettings", HTTP_GET, sendAnalysisConfig);
  server.on("/analysis/saveSettings", HTTP_POST, onSaveConfig);
  server.on("/analysis/editSetting", HTTP_PATCH, onEditSetting);
  server.on("/analysis/deleteModule", HTTP_DELETE, onDeleteModule);
  server.on("/analysis/addModule", HTTP_POST, onAddModule);

  // Make /assets/ public for the server
  server.serveStatic("/", SD, "/assets/");
  server.onNotFound(onNotFoundHandler);
}

/**
 * @brief Finds the index.html file for the web app on the
 *        SD card and sends it.
 */
void WebInterface::sendWebApp()
{
  File entryFile = FileSys::getFile("/index.html");

  if (entryFile)
  {
    server.streamFile(entryFile, TEXT_HTML);
    entryFile.close();
  }
  else
    send(HTTP_NOT_FOUND, TEXT_PLAIN, "File not found");
}

/**
 * @brief Sends 404 error when URI endpoint is not defined.
 *        If not defined, it searches the SD card for a matching file name
 *        and sends it.
 */
void WebInterface::onNotFoundHandler()
{
  // Needed to bypass some CORS requests
  if (server.method() == HTTP_OPTIONS)
  {
    send(HTTP_ACCEPTED);
    
    return;
  }

  DEBUG_PRINTLN("DEBUG: onFoundHandler() called");
  const String Path = server.uri();

  if (!FileSys::exists(Path))
    send(HTTP_NOT_FOUND, TEXT_PLAIN, "404: Not found");
  
  else
  {
    File file = FileSys::getFile(Path);

    if (file)
    {
      DEBUG_PRINTF("DEBUG: file name %s found\n", file.path());
      server.streamFile(file, getContentType(Path));
      file.close();
    }
    else
      send(HTTP_NOT_FOUND, TEXT_PLAIN, "Not found");
  }
}

/**
 * @brief Gets scanned networks, packages the SSIDs,
 *        and sends the data.
 */
void WebInterface::onScanNetworks()
{
  String json;
  JsonDocument doc;
  std::set<String> networks;

  Networking::scanAvailableNetworks(networks);

  // Convert networks into json for frontend to parse
  JsonArray jsonNetworks = doc["ssid"].to<JsonArray>();

  for (const auto &ssid : networks)
    jsonNetworks.add(ssid);

  serializeJson(doc, json);
  send(HTTP_OK, APP_JSON, json);
}

/**
 * @brief Gets the user selected network SSID and password,
 *        attempts to connect to the network and sends the response status.
 */
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
  if (hasConnected)
  {
    resStatus = HTTP_ACCEPTED;
    HapticSettings::Instance().prepareSDWrite(WIFI_SETTINGS_PATH, Networking::getSettings());
  }
  send(resStatus);
}

/**
 * @brief Gets the networking info from the ESP32 and sends it as a JSON object
 */
void WebInterface::onGetNetworkInfo()
{
  String output;
  JsonDocument doc;
  JsonObject networkInfo = doc["info"].to<JsonObject>();

  Networking::getNetworkInfo(networkInfo);

  serializeJson(doc, output);
  send(HTTP_OK, APP_JSON, output);
}

/**
 * @brief Handles the API call for saving access point settings
 */
void WebInterface::onSaveAPSettings()
{
  JsonDocument payload;
  int resStatus = HTTP_UNPROCESSABLE;
  bool success = false;

  if (parsePayload(payload))
  {
    const String ApSsid = payload["newApSsid"] | "";
    const String ApPassword = payload["newApPassword"] | "";
    success = Networking::setAccessPointCredentials(ApSsid, ApPassword);
  }
  if (success)
  {
    resStatus = HTTP_OK;
    HapticSettings::Instance().prepareSDWrite(WIFI_SETTINGS_PATH, Networking::getSettings());
  }
  send(resStatus);
}

/**
 * @brief Handles the API call for forgetting and disconnecting the external WiFi source
 */
void WebInterface::onForgetWiFi()
{
  Networking::forgetExternalWiFi();
  HapticSettings::Instance().prepareSDWrite(WIFI_SETTINGS_PATH, Networking::getSettings());
  send(HTTP_OK);
}

/**
 * @brief Restores the networking settings for the device. Changes take place on device restart
 */
void WebInterface::onResetNetworkSettings()
{
  Networking::setDefaultSettings();
  HapticSettings::Instance().prepareSDWrite(WIFI_SETTINGS_PATH, Networking::getSettings());
  send(HTTP_OK);
}

/**
 * @brief Parses the current AnalysisConfig settings into a JSON string
 *        to send in response to the HTTP request.
 */
void WebInterface::sendAnalysisConfig()
{
  DEBUG_PRINTLN("DEBUG: Sending analysis config settings");

  String json;
  JsonDocument doc;
  JsonObject global = doc["global"].to<JsonObject>();
  JsonArray modulesList = doc["modules"].to<JsonArray>();
  auto curConfig = HapticSettings::Instance().getConfig_mut();

  Utils::packageGlobalSettings(global, curConfig.get());
  Utils::packageModulesList(modulesList, curConfig.get());

  serializeJson(doc, json);
  send(HTTP_OK, APP_JSON, json);
}

/**
 * @brief Saves the current analysis settings into the SD card on the next update loop
 *        in loop()
 */
void WebInterface::onSaveConfig()
{
  DEBUG_PRINTLN("DEBUG: Save config requested");
  
  String json;
  JsonDocument payload;
  int resStatus = HTTP_UNPROCESSABLE;
  bool hasUpdated = false;

  if (parsePayload(payload))
  {
    JsonDocument doc;
    JsonObject global = doc["global"].to<JsonObject>();
    JsonArray modules = doc["modules"].to<JsonArray>();
    auto curConfig = HapticSettings::Instance().getConfig_mut();

    // Populate doc with the current settings
    Utils::packageGlobalSettings(global, curConfig.get());
    Utils::packageModulesList(modules, curConfig.get());
    doc["name"] = payload["name"];

    // Insert the file path and data into the write buffer
    serializeJson(doc, json);
    String filePath = String("/data/") + payload["name"].as<String>() + String(".json");
    HapticSettings::Instance().prepareSDWrite(filePath, json);

    resStatus = HTTP_OK;
  }

  send(resStatus);
}

/**
 * @brief Adds a message to the haptic settings queue for real-time updates
 */
void WebInterface::onEditSetting()
{
  limitReqRate(500u);

  JsonDocument payload;
  int resStatus = HTTP_UNPROCESSABLE;
  bool hasUpdated = false;

  if (parsePayload(payload))
  {
    QueueMessage msg {};
    auto data = payload.as<JsonObject>();
    auto type = static_cast<QueueMsgId>(payload["type"].as<uint>());

    Utils::createMessage(type, data, msg);

    if (HapticSettings::Instance().addMessage(&msg))
      hasUpdated = true;
  }
  if (hasUpdated)
    resStatus = HTTP_OK;

  send(resStatus);
}

/**
 * @brief Adds a message to the haptic settings queue to delete a module on a given output in real-time.
 */
void WebInterface::onDeleteModule()
{
  if (!server.hasArg("outputNumber")) {
    send(HTTP_UNPROCESSABLE);
    return;
  }
  const int OutputNumber = server.arg("outputNumber").toInt();
  QueueMessage msg = {
    .id = QueueMsgId::DeleteModule,
    .module = { .outputNumber = OutputNumber }
  };
  (void) HapticSettings::Instance().addMessage(&msg);

  send(HTTP_OK);
}

/**
 * @brief Adds a message to the haptic settings queue to add a module in real-time.
 */
void WebInterface::onAddModule()
{
  JsonDocument payload;
  int res = HTTP_UNPROCESSABLE;
  
  if (parsePayload(payload))
  {
    auto data = payload.as<JsonObject>();

    // NOTE: Relies on Utils::createModule to create a module with default settings
    QueueMessage msg = {
      .id = QueueMsgId::CreateModule,
      .module = { 
        .outputNumber  = data["outputNumber"].as<int>(),
        .value = { .i = data["type"].as<int>() }
      }
    };
    if (HapticSettings::Instance().addMessage(&msg))
      res = HTTP_OK;
  }
  send(res);
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

/**
 * @brief Formats the data section of the HTTP request into
 *        the passed in reference to the Json structure.
 * 
 * @param output - Reference to the Json structure to populate
 *
 * @return Bool indicating if the data exists and was parsed correctly 
 */
static bool parsePayload(JsonDocument &output)
{
  // "plain" is used to specify the request body holding the data
  if (!server.hasArg("plain"))
    return false;

  String body = server.arg("plain");
  const auto ParsingError = deserializeJson(output, body);

  if (ParsingError)
  {
    DEBUG_PRINTLN("DEBUG: parsing error in parsePayload");

    return false;
  }
  return true;
}

/**
 * @brief Limits the number of requests to 1/Time_ms.
 * 
 * @param Time_ms - Wait time before another request can be processed.
 */
inline void limitReqRate(const unsigned long Time_ms)
{
  static unsigned long lastReq_ms = 0;
  auto now = millis();

  if (now - lastReq_ms < Time_ms)
  {
    send(HTTP_TOO_MANY_REQUESTS);
    return;
  }
  lastReq_ms = now;
}

#ifdef DEV_MODE_EN

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
  server.on("/dev/upload", HTTP_POST, []()
  {
      send(HTTP_OK, TEXT_PLAIN, "Successfully uploaded file");
  }, uploadFile);
  server.on("/dev/printFiles", HTTP_POST, printFiles);
  server.on("/dev/clearSd", HTTP_POST, clearSd);
  server.on("/dev/getMemory", HTTP_GET, getMemory);
}

/**
 * @brief Uploads the user selected file to the desired directory.
 * 
 */
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

    DEBUG_PRINTF("DEBUG: Uploading to: %s\n", fullPath.c_str());
    _uploadFile = FileSys::getFile(fullPath, FILE_WRITE);
  } 
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (_uploadFile)
      _uploadFile.write(upload.buf, upload.currentSize);
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (_uploadFile)
    {
      _uploadFile.close();
      DEBUG_PRINTF("DEBUG: Upload Success: %u bytes\n", upload.totalSize);
    }
  }
}

/**
 * @brief Traverses the SD card files recursively and prints the name and location.
 * 
 */
void WebInterface::printFiles()
{
  DEBUG_PRINTLN("DEBUG: Printing files...");
  File root = FileSys::getFile();

  if (root)
  {
    FileSys::traverseFiles(root, FileSys::printFile);

    send(HTTP_OK, TEXT_PLAIN, "Printed files to serial monitor");
    root.close();
  }
  else
    send(HTTP_BAD_REQUEST, TEXT_PLAIN, "Invalid root provided");
}

/**
 * @brief Traverses the SD files recursively and removes all files.
 * 
 */
void WebInterface::clearSd()
{
  DEBUG_PRINTLN("DEBUG: Clearing SD memory...");
  File root = FileSys::getFile();

  if (root)
  {
    FileSys::traverseFiles(root, FileSys::removeFile);

    root.close();
    send(HTTP_OK, TEXT_PLAIN, "SD File System Cleared");
  }
  else
    send(HTTP_BAD_REQUEST, TEXT_PLAIN, "Invalid root provided");
}

/**
 * @brief Prints heap memory stats to the serial monitor
 */
void WebInterface::getMemory()
{
  // Tracks the largest block to provide info on heap fragmentation as well
  // The more fragmented, the slower the code runs
  static size_t lastMaxBlock = 0;
  size_t currentMaxBlock = ESP.getMaxAllocHeap();

  DEBUG_PRINTF("Free Heap: %u | Max Block: %u | Diff: %d\n", 
                ESP.getFreeHeap(), 
                currentMaxBlock, 
                (int)(currentMaxBlock - lastMaxBlock));
  
  lastMaxBlock = currentMaxBlock;

  send(HTTP_OK);
}

#endif // DEV_MODE_EN
