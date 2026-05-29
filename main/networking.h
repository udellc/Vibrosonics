/***************************************************************
 * FILE: networking.h
 * 
 * DATE: 11/18/2025
 * 
 * DESCRIPTION: This namespace contains functions to initialize 
 * and manage Wi-Fi connectivity, enabling network communication
 * capabilities for the ESP32.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#ifndef NETWORKING_H
#define NETWORKING_H

#include <set>
#include <Arduino.h>
#include <ArduinoJson.h>

namespace Networking
{
  //! Initializes the Wi-Fi settings for the web app
  bool init();

  //! Uses built-in Wi-Fi to access web URL
  bool initAccessPoint(const String &Ssid, const String &Password);

  //! Disconnects the ESP32 access point and attempts to reconnect to the new network
  bool connectToNetwork(const String &Ssid, const String &Password);

  //! Scans available networks and adds their SSID to the result vector
  void scanAvailableNetworks(std::set<String> &result);

  //! Gets the networking info from the ESP32 and populates the object
  void getNetworkInfo(JsonObject& info);

  //! Clears the external Wi-Fi credentials and disconnects if we can
  void forgetExternalWiFi();

  //! Updates the on-device Wi-Fi credentials
  bool setAccessPointCredentials(const String &Ssid, const String &Password);

  //! Restores default networking settings
  void setDefaultSettings();

  //! Writes the settings document to the SD card
  //! NOTE: Use for the external DACs since the SD card also shares the same SPI pins.
  //        We need to control exactly when to write data to the SD card
  void writeSettings();
}

#endif
