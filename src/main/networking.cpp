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

#ifdef DEV_MODE_EN
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
        DEBUG_PRINTLN("DEBUG: Successfully connected to saved Wi-Fi");
        
        return true;
      }
      else
      {
        DEBUG_PRINTLN("WARNING: Could not connect to saved WiFi");
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
  DEBUG_PRINTLN("DEBUG: Starting WiFi access point...");

  bool success = WiFi.softAP(ApSSID, ApPassword);
  success &= MDNS.begin(DefaultHostname);

  if (!success)
  {
    DEBUG_PRINTLN("DEBUG: Access point creation failed.");
  }
  else
  {
    DEBUG_PRINT("DEBUG: Access point created. Accessible at ");
    DEBUG_PRINT(WiFi.softAPIP());
    DEBUG_PRINTF(" or http://%s\n", DefaultHostname);
  }
  return success;
}

/**
 * @brief Scans for discoverable networks on ESP32 and stores the found
 *        SSIDs in the result parameter.
 * 
 * @param result - Reference to the set to be populated
 * 
 * NOTE: A set is used over a vector because the ESP32 can sometimes scan the same network twice.
 */
void Networking::scanAvailableNetworks(std::set<String> &result)
{
  const auto NumNetworks = WiFi.scanNetworks();

  if (NumNetworks == 0)
  {
    DEBUG_PRINTLN("DEBUG: No networks");
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

/**
 * @brief Blocking function to connect to the desired network, saving the credentials
 *        on success.
 * 
 * @param Ssid - Name of the network to connect to
 * @param Password - Password for the SSID
 *  
 * @return Bool indicating if the network was successfully connected to 
 */
bool Networking::connectToNetwork(const String &Ssid, const String &Password)
{
  int numTries = 0u;
 
  if (Ssid.length() == 0)
  {
    DEBUG_PRINTLN("WARNING: Empty SSID provided");
    return false;
  }
  DEBUG_PRINTF("DEBUG: Attempting to connect to %s\n", Ssid);
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
    (void) FileSys::writeFile(WIFI_SETTINGS_PATH, JsonWifi);
    DEBUG_PRINTF("DEBUG: Successfully connected to %s and saved info to SD card\n");

    return true;
  }
  DEBUG_PRINTF("DEBUG: Connection attempt to %s failed\n", Ssid);
  return false;
}

/**
 * @brief Getter for the name of the current network SSID
 * 
 * @return String indicating the connected SSID
 */
String Networking::getNetworkSsid()
{
  return (wifiStatus == Status_T::ConnectedToWiFi) ? currentWifi.ssid : String(ApSSID);
}
