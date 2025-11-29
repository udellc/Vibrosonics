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

const char *defaultHostname = "vibrosonics";
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
    Serial.print(WiFi.softAPIP());
    Serial.printf(" or http://%s\n", defaultHostname);
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

// TODO: implement
bool Networking::connectToNetwork(const String &Ssid, const String &Password)
{
  const unsigned long MaxTimeout_ms = 4000u;
  unsigned long prev_ms = millis();
  unsigned long current_ms = prev_ms;

  WiFi.scanDelete();
  WiFi.disconnect();
  WiFi.begin(Ssid.c_str(), Password.c_str());

  while ( (WiFi.status() != WL_CONNECTED) && ( (current_ms - prev_ms) <= MaxTimeout_ms) )
  {
    current_ms = millis();
    delay(100);
  }
  return (WiFi.status() == WL_CONNECTED) ? true : false;
}
