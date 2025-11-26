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

/**
 * @brief Initializes the web server in either Upload or web app mode,
 *        depending on the config file
 * 
 * @return Bool inidicating if the web server is live
 */
bool WebServer::init()
{
  bool success = true;

  Serial.println("Starting web server...");

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

/**
 * @brief Helper function for getting the content type of a file, given the file name
 * 
 * @param Path - Name of the file we want to get the type for
 * 
 * @return String for the content type of the file 
 */
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

/**
 * @brief Connects the backend API endpoints to the respective handlers
 * 
 * NOTE: This function should be the only time SD is referenced outside of the FileSys namespace
 */
inline void WebServer::setupWebApp()
{
  // Lambda for the initial request to the URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
  {
    req->send(SD, "/index.html", getContentType("/index.html"));
  });
  server.serveStatic("/", SD, "/");
}

#ifdef UPLOAD_MODE
static const char *uploadForm PROGMEM = R"(
<!DOCTYPE html>
  <html>
  <head>
    <title>Upload Mode</title>
  </head>
  <body>
    <h1>In Upload Mode</h1>
    <h3>Upload a File</h3>
    <form method='POST' action='/upload' enctype='multipart/form-data'>
      <label for='directoryName'>Target Directory</label>
      <input type='text' name='directoryName' value='/'>
      
      <label for='fileName'>Target File</label>
      <input type='file' name='fileName'>
      
      <input type='submit' value='Upload'>
    </form>
    <form method='GET' action='/printFiles'>
      <label for='printFiles'>See root directory content</label>
      <input type='submit' value='Print Files'>
    </form>
  </body>
  </html>
)";

/**
 * @brief Adds endpoint API handlers for upload mode
 * 
 */
inline void WebServer::setupUploadMode()
{
  server.on("/", HTTP_GET, sendUploadPage);
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *req)
  {
    req->send(HTTP_OK, "text/plain", "Upload Successful");
  }, handleUpload);
  server.on("/printFiles", HTTP_GET, printFiles);

  // TODO: add a make directory API for the assets directory produced by Vite
}

/**
 * @brief Sends the upload file form
 * 
 * @param req - Object to respond to the initial request
 */
void WebServer::sendUploadPage(AsyncWebServerRequest *req)
{
  req->send(HTTP_OK, "text/html", uploadForm);
}

/**
 * @brief Uploads the request file into the specified directory name on the SD card in chunks
 * 
 * @param req - Object to handle the request
 * @param filename - File we want to download
 * @param index - Current write index of filename
 * @param data - Chunk of data to write
 * @param len - Total file size
 * 
 * NOTE: Ref: https://github.com/lacamera/ESPAsyncWebServer?tab=readme-ov-file#file-upload-handling
 */
void WebServer::handleUpload(AsyncWebServerRequest *req, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  String path = req->arg("directoryName");

  // Path handling branches
  if (path.isEmpty())
  {
    return;
  }
  else if (path == "/")
  {
    path += filename;
  }
  else
  {
    path += String("/") + filename;
  }

  // File uploading branches
  if (!index)
  {
    Serial.print("Starting upload: ");
    Serial.println(path);
    req->_tempFile = SD.open(path, FILE_WRITE);
  }
  if (len)
  {
    req->_tempFile.write(data, len);
  }
  if (final)
  {
    req->_tempFile.close();
  }
}

/**
 * @brief Prints the contents of the root directory from the SD card into the serial monitor 
 *
 * @param req - Handler for the web request
 */
void WebServer::printFiles(AsyncWebServerRequest *req)
{
  File root = FileSys::getFile();

  if (root)
  {
    FileSys::traverseFiles(root, FileSys::printFile);

    root.close();
    req->send(HTTP_OK, "text/plain", "SD File System Printed");
  }
  else
  {
    req->send(HTTP_BAD_REQUEST, "text/plain", "Invalid root provided");
  }
}

#endif
