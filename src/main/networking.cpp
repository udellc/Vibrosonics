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

// Networking globals
#define WIFI_SETTINGS_PATH "/data/wifiSettings.json"
#define WIFI_CONNECTION_DELAY_INTERVAL_MS 500u
#define MAX_CONNECTION_TRIES 10u

#ifdef DEV_MODE
  const char *ApSSID = "Vibrosonics-Dev";
#else
  const char *ApSSID = "Vibrosonics-Unsecure";
#endif
const char *DefaultHostname = "vibrosonics";
const char *ApPassword = "1234567890";

struct WiFiInfo
{
  String ssid;
  String password;
};

WiFiInfo currentWifi;
Networking::Status_T wifiStatus;
static JsonDocument settingsDoc;

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
  // Prevents Flash write when WiFi.begin() is called
  WiFi.persistent(false);

  // Operate in AP and station mode
  WiFi.mode(WIFI_AP_STA);
  const bool HasSettings = FileSys::exists(WIFI_SETTINGS_PATH);

  if (HasSettings)
  {
    File settingsFile = FileSys::getFile(WIFI_SETTINGS_PATH);
    const auto Error = deserializeJson(settingsDoc, settingsFile);
    settingsFile.close();

    if (!Error)
    {
      auto ssid = settingsDoc["ssid"];
      auto password = settingsDoc["password"];

      if (connectToNetwork(ssid, password))
      {
        currentWifi.ssid = String(ssid);
        currentWifi.password = String(password);
        wifiStatus = Status_T::ConnectedToWiFi;
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
  const auto NumNetworks = WiFi.scanNetworks();

  if (NumNetworks == 0)
  {
    Serial.println("No networks");
  }
  else
  {
    for (auto i = 0u; i < NumNetworks; i++)
    {
      result.insert(WiFi.SSID(i));
    }
  }
  WiFi.scanDelete();
}

// TODO: add header comment
bool Networking::connectToNetwork(const String &Ssid, const String &Password)
{
  int numTries = 0u;
 
  if (Ssid.length() == 0)
  {
    Serial.println("Empty SSID provided");
    return false;
  }
  Serial.printf("Attempting to connect to %s\n", Ssid);
  WiFi.begin(Ssid.c_str(), Password.c_str());

  // Wait for connection with 10 sec timeout
  if (WiFi.waitForConnectResult(10000) == WL_CONNECTED)
  {
    (void) MDNS.begin(DefaultHostname);

    wifiStatus == Status_T::ConnectedToWiFi;
    currentWifi.ssid = Ssid;
    currentWifi.password = Password;
    const String JsonWifi = "{\n"
                              "  \"ssid\": \"" + currentWifi.ssid + "\",\n"
                              "  \"password\": \"" + currentWifi.password + "\"\n"
                              "}";
    FileSys::writeFile(WIFI_SETTINGS_PATH, JsonWifi);

    return true;
  }
  Serial.printf("Connection attempt to %s failed\n", Ssid);
  return false;
}

String Networking::getNetworkSsid()
{
  return (wifiStatus == Status_T::ConnectedToWiFi) ? currentWifi.ssid : String(ApSSID);
}
