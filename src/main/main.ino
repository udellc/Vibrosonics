/***************************************************************
 * FILE: main.ino
 * 
 * DATE: 11/4/2025
 * 
 * DESCRIPTION: Entry point for starting the Vibrosonics audio
 * analysis and web app.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#include "webServer.h"
#include "networking.h"
#include "fileSys.h"
#include "config.h"

bool boot();

/**
 * @brief 
 * 
 */
void setup()
{
  Serial.begin(115200);

  // On setup failure, do nothing 
  if (!boot())
  {
    Serial.println("Setup failure. Looping...");
    
    while (1)
      delay(1000);
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
 * @return success - Bool indicating of all components initialized successfully
 */
bool boot()
{
  bool success = true;
  
  success &= FileSys::init();
  success &= Networking::initAccessPoint();
  success &= WebServer::init();

  return success;
}