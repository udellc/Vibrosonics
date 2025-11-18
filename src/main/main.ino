/***************************************************************
 * FILE: main.ino
 * 
 * DATE: 11/4/2025
 * 
 * DESCRIPTION: Entry point for starting the web server on 
 * the ESP32
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#include <WiFi.h>
#include <AsyncTCP.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

// TODO: maybe move to a different file
String getContentType(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".css"))  return "text/css";
  if (path.endsWith(".js"))   return "application/javascript";
  if (path.endsWith(".png"))  return "image/png";
  if (path.endsWith(".jpg"))  return "image/jpeg";
  if (path.endsWith(".ico"))  return "image/x-icon";
  if (path.endsWith(".json")) return "application/json";
  return "text/plain";
}

// TODO: maybe move to a different file
void handleLandingPage(AsyncWebServerRequest *req, String path)
{
  if (path.endsWith("/"))
  {
    path += "index.html";
  }
  if (LittleFS.exists(path))
  {
    req->send(LittleFS, path, getContentType(path));
  }
  else
  {
    Serial.println("index.html not sent");
  }
}

// TODO: maybe move to different file
bool initFileSystem()
{
  const bool Success = LittleFS.begin();
  if (!Success)
  {
    Serial.println("Mini file system not initialized");
  }
  else
  {
    Serial.println("Mini file system initialized");
  }
  return Success;
}

// TODO: maybe move to a different file
bool initWiFiConnection(const char *SSID, const char *Password, const uint MaxTimeout_ms)
{
  const bool IsEmptyCredentials = (SSID == "") || (Password == "");
  // unsigned long curTime = millis();
  // unsigned long prevTime = curTime;

  if (IsEmptyCredentials)
  {
    Serial.println("Empty WiFi credentials");
    return false;
  }
  Serial.println("Connecting to WiFi...");
  WiFi.begin(SSID, Password);

  // TODO: add a timer using MaxTimeout_ms
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.SSID());
  Serial.println(WiFi.localIP());

  return true;
}

void setup()
{
  const uint MaxTimeout_ms = 3000;
  bool success = true;

  Serial.begin(115200);
  
  success &= initFileSystem();

  // FIXME: fill the first arg with wifi SSID and second with the password
  success &= initWiFiConnection("", "", MaxTimeout_ms);

  if (!success)
  {
    Serial.println("Setup failure");
    return;
  }
  // Lambda for req
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
  {
    handleLandingPage(req, req->url());
  });
  server.begin();
}

void loop()
{

}
