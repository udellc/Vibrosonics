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

// TODO: set this to your local network this when done
const char *SSID = "";
const char *Password = "";

AsyncWebServer server(80);
String header;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Basic HTML Page</title>
</head>
<body>
    <h1>Hello</h1>
    <p>VibroSonics</p>
</body>
</html>
)rawliteral";

void setup()
{
  Serial.begin(115200);
  
  WiFi.scanNetworks(true);
  
  
  WiFi.begin(SSID, Password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println(WiFi.SSID());
  }
  Serial.println(WiFi.localIP());

  // Lambda for req
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(200, "text/html", index_html);
  });
  server.begin();
}

void loop()
{

}














