/***************************************************************
 * FILE: networking.cpp
 * 
 * DATE: 11/18/2025
 * 
 * DESCRIPTION: The implementation file for the Networking
 * namespace.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#include "networking.h"
#include <WiFi.h>

const char *defaultHostname = "vibrosonics";
const char *apSSID = "VIBROSONICS";
const char *apPassword = "1234567890";

// TODO: add header comment
bool Networking::initAccessPoint()
{  
  WiFi.mode(WIFI_MODE_APSTA);
  Serial.println("Starting WiFi access point...");

  const bool Success = WiFi.softAP(apSSID, apPassword);

  if (!Success)
  {
    Serial.println("Access point creation failed.");
  }
  else
  {
    Serial.print("Access point created. Accessible at ");
    Serial.println(WiFi.softAPIP());
  }

  return Success;
}

void Networking::scanAvailableNetworks()
{

}

bool Networking::connectToNetwork()
{
  return false;
}

bool Networking::disconnectFromNetwork()
{
  return false;
}
