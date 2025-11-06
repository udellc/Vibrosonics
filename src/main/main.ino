/***************************************************************
 * FILE: main.ino
 * 
 * DATE: 11/4/2025
 * 
 * DESCRIPTION: Entry point for starting the web server on 
 *  the ES32
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
// #include "webServer.h"

// FIXME: set this to your local network this when done
const char *SSID = "";
const char *Password = "";

AsyncWebServer server(80);
String header;

void handleLandingPage(AsyncWebServerRequest *req)
{
  AsyncWebServerResponse *res = req->beginResponse(LittleFS, "/index.html", "text/html");
  req->send(res);
}

void setup()
{
  Serial.begin(115200);
  WiFi.begin(SSID, Password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println(WiFi.SSID());
  }
  Serial.println(WiFi.localIP());

  // Lambda for req
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req)
  {
    handleLandingPage(req);
  });
  server.begin();
}

void loop()
{

}
