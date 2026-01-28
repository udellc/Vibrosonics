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
#include "VibrosonicsAPI.h"

static VibrosonicsAPI vapi = VibrosonicsAPI();

/**
 * @brief 
 * 
 */
void setup()
{
  bool success = true;

  Serial.begin(115200);
  success &= FileSys::init();
  success &= Networking::init();
  success &= WebServer::init();

  // On setup failure, do nothing 
  if (!success)
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
