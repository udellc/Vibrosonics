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

static JsonDocument settingsDoc;
volatile bool needsSDWrite = false;

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
  bool success = false;

  // Prevents Flash write when WiFi.begin() is called
  WiFi.persistent(false);

  // Always boot up with AP, then open STA if available
  WiFi.mode(WIFI_AP_STA);

  if (!FileSys::exists(WIFI_SETTINGS_PATH))
  {
    // Create the file for future usage
    FileSys::writeFile(WIFI_SETTINGS_PATH, "");
  }
  // Open the preferences from file on SD card
  File settings = FileSys::getFile(WIFI_SETTINGS_PATH);
  const auto Error = deserializeJson(settingsDoc, settings);
  settings.close();

  if (!Error)
  {
    // Create the AP using data in the file or the default settings
    auto ssid = settingsDoc["apSsid"] | DefaultApSSID;
    auto password = settingsDoc["apPassword"] | DefaultApPassword;
    success = initAccessPoint(ssid, password);

    // Now open the STA for an external WiFi source
    ssid = settingsDoc["extSsid"] | "";
    password = settingsDoc["extPassword"] | "";

    success |= connectToNetwork(ssid, password);
  }
  else
  {
    DEBUG_PRINTLN("WARNING: Could not open WiFi settings file");

    settingsDoc["apSsid"] = DefaultApSSID;
    settingsDoc["apPassword"] = DefaultApPassword;
    success = initAccessPoint(DefaultApSSID, DefaultApPassword);
  }
  return success;
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
    DEBUG_PRINTLN("WARNING: Empty SSID provided for external WiFi source");
    return false;
  }
  DEBUG_PRINTF("DEBUG: Attempting to connect to %s\n", Ssid);
  WiFi.begin(Ssid.c_str(), Password.c_str());

  // Wait for connection with 10 sec timeout
  if (WiFi.waitForConnectResult(10000) == WL_CONNECTED)
  {
    (void) MDNS.begin(DefaultHostname);
    settingsDoc["extSsid"] = Ssid;
    settingsDoc["extPassword"] = Password;
    needsSDWrite = true;

    DEBUG_PRINTF("DEBUG: Successfully connected to %s\n", Ssid);
    return true;
  }
  DEBUG_PRINTF("DEBUG: Connection attempt to %s failed\n", Ssid);
  resetWifi();
  
  return false;
}

/**
 * @brief Retrieves the networking info from the ESP32 and packages
 * the info into a Json object
 * 
 * @param info - Reference to the JsonObject to be populated
 */
void Networking::getNetworkInfo(JsonObject& info)
{
  info["extSsid"] = settingsDoc["extSsid"] | "";
  info["apSsid"] = settingsDoc["apSsid"] | DefaultApSSID;
  info["apPassword"] = settingsDoc["apPassword"] | DefaultApPassword;
  info["rssi"] = WiFi.RSSI();
}

/**
 * @brief Clears the external WiFi credentials and disconnects from it
 */
void Networking::forgetExternalWiFi()
{
  settingsDoc["extSsid"] = "";
  settingsDoc["extPassword"] = "";
  needsSDWrite = true;

  resetWifi();
}

/**
 * @brief Update the settings document and checks for empty credentials
 * 
 * @param Ssid - New AP SSID to use
 * @param Password - New AP password to use 
 * 
 * @return Bool indicating if the credentials are valid 
 */
bool Networking::setAccessPointCredentials(const String& Ssid, const String& Password)
{
  if ((Ssid.length() == 0) || (Password.length() == 0))
  {
    DEBUG_PRINTLN("WARNING: Invalid access point credentials.");
    return false;
  }
  settingsDoc["apSsid"] = Ssid;
  settingsDoc["apPassword"] = Password;
  needsSDWrite = true;

  return true;
}

/**
 * @brief clears the internal settings doc and disconnects from external Wi-Fi
 */
void Networking::setDefaultSettings()
{
  settingsDoc.clear();
  needsSDWrite = true;
  resetWifi();
}

/**
 * @brief Write the settings JSON document to the SD card
 */
void Networking::writeSettings()
{
  if (needsSDWrite)
  {
    DEBUG_PRINTLN("DEBUG: Writing WiFi settings to SD card");
    String data;

    if (settingsDoc.isNull())
      data = "";
    else
      serializeJson(settingsDoc, data);

    FileSys::writeFile(WIFI_SETTINGS_PATH, data);
    needsSDWrite = false;
  }
}

/**
 * @brief Disconnects the external WiFi signal, keeping the AP.
 */
void resetWifi()
{
  WiFi.disconnect(true);
  delay(100u);
  WiFi.mode(WIFI_AP_STA);
}
