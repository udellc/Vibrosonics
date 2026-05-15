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
#include "config.h"
#include "fileSys.h"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <algorithm>

// Networking globals
#define WIFI_SETTINGS_PATH "/data/wifiSettings.json"

const char *DefaultApSSID = "Vibrosonics-Unsecure";
const char *DefaultHostname = "vibrosonics";
const char *DefaultApPassword = "1234567890";

struct WiFiInfo
{
  String ssid;
  String password;
};

WiFiInfo currentWifi;
Networking::Status_T wifiStatus;
static JsonDocument settingsDoc;

// Internal function helpers
static void inline resetWifi();

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
      // Try to connect to external Wi-Fi first
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
      // Didn't work, try to connect to AP mode with user settings
      ssid = settingsDoc["apSsid"];
      password = settingsDoc["apPassword"];

      if (initAccessPoint(ssid, password))
      {
        DEBUG_PRINTLN("DEBUG: Using user saved network settings.");
        return true;
      }
    }
  }
  // Fallback to default AP mode
  if (!initAccessPoint(DefaultApSSID, DefaultApPassword))
  {
    DEBUG_PRINTLN("FATAL: Could not use any Wi-Fi signal");
    wifiStatus = Status_T::NotConnected;
    return false;
  }
  return true;
}

/**
 * @brief Initializes WiFi capabilities on the ESP32 in access point mode, with a custom host name
 * 
 * @param Ssid - Name of the Wi-Fi to 
 * 
 * @return Bool indicating of the WiFi access point has been created with the
 *         defaultHostname domain.
 */
bool Networking::initAccessPoint(const String& Ssid, const String& Password)
{
  if (Ssid.length() == 0)
  {
    DEBUG_PRINTLN("FATAL: SSID for AP mode is empty.");
    return false;
  }
  DEBUG_PRINTLN("DEBUG: Starting WiFi access point...");

  bool success = WiFi.softAP(Ssid, Password);
  success &= MDNS.begin(DefaultHostname);

  if (!success)
  {
    DEBUG_PRINTLN("FATAL: Access point creation failed.");
  }
  else
  {
    currentWifi.ssid = DefaultApSSID;
    currentWifi.password = DefaultApPassword;
    wifiStatus = Status_T::ConnectedToAP;

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
  DEBUG_PRINTLN("DEBUG: Getting networks...");

  if (NumNetworks == 0)
  {
    DEBUG_PRINTLN("DEBUG: No networks found");
  }
  else
  {
    DEBUG_PRINTF("DEBUG: %d networks found\n", NumNetworks);
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
    DEBUG_PRINTF("DEBUG: Successfully connected to %s and saved info to SD card\n", currentWifi.ssid);

    return true;
  }
  DEBUG_PRINTF("DEBUG: Connection attempt to %s failed\n", Ssid);
  resetWifi();
  
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

/**
 * @brief Restarts the WiFi signal from the ESP32 in AP and STation mode.
 *
 * NOTE: This function should only be called after failing to connect to an alternative network
 */
void resetWifi()
{
  WiFi.disconnect(true);
  delay(100u);
  WiFi.mode(WIFI_AP_STA);
}
