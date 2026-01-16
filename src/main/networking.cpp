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

static TimerHandle_t wifiTimer;
static SemaphoreHandle_t wifiMutex;
Networking::Status wifiStatus;

bool Networking::init()
{
  // TODO: Init access point if no WiFi settings found, else use saved settings
  bool success = initAccessPoint();

  if (success) wifiStatus = Status::ConnectedToAP;
  else wifiStatus = Status::NotConnected;

  wifiMutex = xSemaphoreCreateMutex();
  wifiTimer = xTimerCreate
  (
    "WiFiTimer",
    pdMS_TO_TICKS(200),
    pdTRUE,   // Auto re-trigger.
    nullptr,  // Timer ID pointer, not used.
    initWifiTimerConnect
  );
  return success;
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
void Networking::initWifiTimerConnect(TimerHandle_t timer)
{
  if (xSemaphoreTake(wifiMutex, pdMS_TO_TICKS(50)) == pdTRUE)
  {
    if (wifiStatus == Status::JoiningWiFi)
    {
      wl_status_t status = WiFi.status();

      if (status == WL_CONNECTED)
      {
        wifiStatus = Status::ConnectedToWiFi;
        xTimerStop(wifiTimer, 0);
      }
      else if (status != WL_IDLE_STATUS && status != WL_CONNECT_FAILED && status != WL_NO_SHIELD)
      {
        wifiStatus = NotConnected;
        xTimerStop(wifiTimer, 0);
      }
    }
    xSemaphoreGive(wifiMutex);
  }
}

// TODO: implement
bool Networking::connectToNetwork(const String &Ssid, const String &Password)
{
  const uint DisconnectDelay_ms = 100u;
  bool success = true;

  WiFi.scanDelete();
  WiFi.disconnect();
  MDNS.end();
  delay(DisconnectDelay_ms);
  WiFi.begin(Ssid.c_str(), Password.c_str());

  if (xSemaphoreTake(wifiMutex, pdMS_TO_TICKS(50)) == pdTRUE)
  {
    wifiStatus = Status::JoiningWiFi;

    if (xTimerStart(wifiTimer, 0) != pdPASS)
    {
      success = false;
    }
    xSemaphoreGive(wifiMutex);
  }
  else
  {
    success = false;
  }
  if (success) 
  {
    (void) MDNS.begin(DefaultHostname);
  }
  return success;
}
