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

#include "networking.h"
#include "fileSys.h"
#include "webServer.h"

AsyncWebServer server(80);

//---------------------------------------------
// Prototypes
bool boot();

/**
 * @brief 
 * 
 */
void setup()
{
  Serial.begin(115200);

  if (!boot())
  {
    Serial.println("Setup failure");
  }
}

/**
 * @brief 
 * 
 */
void loop()
{

}

/**
 * @brief boot() initializes the major components needed for the WiFI web app to run
 * 
 * @return success - Bool indicating of all components initializes successfully 
 */
bool boot()
{
  bool success = true;
  
  success &= FileSys::init();
  success &= Networking::initAccessPoint();
  success &= WebServer::init();

  return success;
}