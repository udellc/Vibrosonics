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
#include <ESPmDNS.h>

const char *defaultHostname = "vibrosonics-webapp";
const char *apSSID = "Vibrosonics-Unsecure";
const char *apPassword = "1234567890";

/**
 * @brief Initializes WiFi capabilities on the ESP32 in access point mode, with a custom host name
 * 
 * @return Bool indicating of the WiFi access point has been created with the
 *         defaultHostname domain.
 */
bool Networking::initAccessPoint()
{  
  WiFi.mode(WIFI_MODE_APSTA);
  Serial.println("Starting WiFi access point...");

  bool success = WiFi.softAP(apSSID, apPassword);
  success &= MDNS.begin(defaultHostname);

  if (!success)
  {
    Serial.println("Access point creation failed.");
  }
  else
  {
    Serial.print("Access point created. Accessible at ");
    Serial.println(WiFi.softAPIP());
  }
  return success;
}

// TODO: implement
void Networking::scanAvailableNetworks()
{

}

// TODO: implement
bool Networking::connectToNetwork()
{
  return false;
}

// TODO: implement
bool Networking::disconnectFromNetwork()
{
  return false;
}
