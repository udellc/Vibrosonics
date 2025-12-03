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

const char *DefaultHostname = "vibrosonics";
const char *ApSSID = "Vibrosonics-Unsecure";
const char *ApPassword = "1234567890";

// TODO: implment and use this instead of the initAccessPoint in main.ino : boot
//       Needs to be able to attempt to reconnect to saved WiFi settings, is fails, then use initAccessPoint
bool Networking::init()
{
  return true;
}

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

  bool success = WiFi.softAP(ApSSID, ApPassword);
  success &= MDNS.begin(DefaultHostname);

  if (!success)
  {
    Serial.println("Access point creation failed.");
  } 
  else
  {
    Serial.print("Access point created. Accessible at ");
    Serial.print(WiFi.softAPIP());
    Serial.printf(" or http://%s\n", DefaultHostname);
  }
  return success;
}

// TODO: add header comment
void Networking::scanAvailableNetworks(std::vector<String> &result)
{
  const int16_t NumNetworks = WiFi.scanNetworks();

  if (NumNetworks == 0)
  {
    Serial.println("No networks");
  }
  else
  {
    // TODO: use a data structure to store networks and return it
    for (int16_t i = 0; i < NumNetworks; i++)
    {
      result.push_back(WiFi.SSID(i));
    }
    WiFi.scanDelete();
  }
}

// TODO: add header comment
bool Networking::connectToNetwork(const String &Ssid, const String &Password)
{
  // const unsigned long MaxTimeout_ms = 00u;
  const uint DisconnectDelay_ms = 100u;

  WiFi.scanDelete();
  WiFi.disconnect();
  MDNS.end();
  delay(DisconnectDelay_ms);
  WiFi.begin(Ssid.c_str(), Password.c_str());

  const uint8_t Status = WiFi.waitForConnectResult();
  const bool IsConnected = (Status == WL_CONNECTED);
  bool success = IsConnected;

  if (IsConnected)
  {
    success &= MDNS.begin(DefaultHostname);
  }
  Serial.printf("Done, has res %d\n", Status);

  return success;
}
