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
#include "fileSys.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>

#define WIFI_SETTINGS_PATH "/data/wifiSettings.json"
#define MAX_CONNECTION_TRIES 5u

struct WiFiInfo
{
  String ssid;
  String password;
};

WiFiInfo currentWifi;
static JsonDocument settingsDoc;

#ifdef DEV_MODE
  const char *ApSSID = "Vibrosonics-Dev";
#else
  const char *ApSSID = "Vibrosonics-Dev";
#endif

const char *DefaultHostname = "vibrosonics";
const char *ApPassword = "1234567890";

static TimerHandle_t wifiTimer;
static SemaphoreHandle_t wifiMutex;
volatile static Networking::Status_T wifiStatus;

/**
 * @brief Initializes the WiFi settings, using saved settings if they exist.
 *        If settings don't exist, falls back in AP mode.
 * 
 * @return True if the system was able to either connect to a 
 *         saved WiFi network or start in AP mode.
 *         False otherwise.
 */
bool Networking::init()
{
  WiFi.mode(WIFI_MODE_APSTA);
  wifiMutex = xSemaphoreCreateMutex();
  wifiTimer = xTimerCreate
  (
    "WiFiTimer",
    pdMS_TO_TICKS(200),
    pdTRUE,   // Auto re-trigger.
    nullptr,  // Timer ID pointer, not used.
    initiateWifiTimerConnect
  );
  const bool HasSettings = FileSys::exists(WIFI_SETTINGS_PATH);

  // Try connecting to a saved network
  if (HasSettings)
  {
    auto settingsFile = FileSys::getFile(WIFI_SETTINGS_PATH);
    const auto Error = deserializeJson(settingsDoc, settingsFile);
    settingsFile.close();

    if (!Error)
    {
      auto ssid = settingsDoc["ssid"];
      auto password = settingsDoc["password"];

      (void) connectToNetwork(ssid, password);
      uint numTries = 0u;
      
      while (wifiStatus != Status_T::ConnectedToWiFi && numTries != MAX_CONNECTION_TRIES)
      {
        numTries++;
        delay(1000u);
      }
      if (wifiStatus == Status_T::ConnectedToWiFi)
      {
        currentWifi.ssid = String(ssid);
        currentWifi.password = String(password);
        Serial.println("Successfully connected to saved Wi-Fi");
        return true;
      }
    }
  }
  // Fall back on AP mode if saved network settings DNE or couldn't connect
  if (initAccessPoint())
  {
    currentWifi.ssid = "";
    currentWifi.password = "";

    wifiStatus = Status_T::ConnectedToAP;
    return true;
  }
  // Complete failure
  wifiStatus = Status_T::NotConnected;
  return false;
}

/**
 * @brief Initializes WiFi capabilities on the ESP32 in access point mode, with a custom host name
 * 
 * @return Bool indicating of the WiFi access point has been created with the
 *         defaultHostname domain.
 */
bool Networking::initAccessPoint()
{
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
void Networking::scanAvailableNetworks(std::set<String> &result)
{
  const int16_t NumNetworks = WiFi.scanNetworks();

  if (NumNetworks == 0)
  {
    Serial.println("No networks");
  }
  else
  {
    for (int16_t i = 0; i < NumNetworks; i++)
    {
      result.insert(WiFi.SSID(i));
    }
    WiFi.scanDelete();
  }
}

// TODO: add header comment
void Networking::initiateWifiTimerConnect(TimerHandle_t timer)
{
  if (xSemaphoreTake(wifiMutex, pdMS_TO_TICKS(50)) == pdTRUE)
  {
    if (wifiStatus == Status_T::JoiningWiFi)
    {
      wl_status_t status = WiFi.status();

      if (status == WL_CONNECTED)
      {
        Serial.println("Connected to WiFi");

        wifiStatus = Status_T::ConnectedToWiFi;
        xTimerStop(wifiTimer, 0);
      }
      else if (status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL || status == WL_CONNECTION_LOST)
      {
        Serial.printf("WiFi Connection Failed (Status: %d)\n", status);
        wifiStatus = Status_T::NotConnected; 
        xTimerStop(wifiTimer, 0);
      }
      // Safety: If it's not connected and not currently trying (Idle/No Shield), abort
      else if (status != WL_IDLE_STATUS && status != WL_DISCONNECTED)
      {
        wifiStatus = Status_T::NotConnected;
        xTimerStop(wifiTimer, 0);
      }
    }
    xSemaphoreGive(wifiMutex);
  }
}

// TODO: add header comment
bool Networking::connectToNetwork(const String &Ssid, const String &Password)
{
  const uint DisconnectDelay_ms = 100u;
  bool isMonitored = true;

  WiFi.scanDelete();
  WiFi.disconnect();
  MDNS.end();
  delay(DisconnectDelay_ms);
  WiFi.begin(Ssid.c_str(), Password.c_str());

  if (xSemaphoreTake(wifiMutex, pdMS_TO_TICKS(50)) == pdTRUE)
  {
    wifiStatus = Status_T::JoiningWiFi;

    if (xTimerStart(wifiTimer, 0) != pdPASS)
    {
      isMonitored = false;
    }
    xSemaphoreGive(wifiMutex);
  }
  else
  {
    isMonitored = false;
  }
  if (isMonitored)
  {
    (void) MDNS.begin(DefaultHostname);

    // TODO: may need to move this into a seperate function if writing to SD card takes too long
    currentWifi.ssid = Ssid;
    currentWifi.password = Password;
    const String JsonWifi = "{\n"
                              "  \"ssid\": \"" + currentWifi.ssid + "\",\n"
                              "  \"password\": \"" + currentWifi.password + "\"\n"
                              "}";
    FileSys::writeFile(WIFI_SETTINGS_PATH, JsonWifi);
  }
  return isMonitored;
}

// TODO: add header comment
String Networking::getNetworkSsid()
{
  return (wifiStatus == Status_T::ConnectedToAP) ? ApSSID : currentWifi.ssid;
}
