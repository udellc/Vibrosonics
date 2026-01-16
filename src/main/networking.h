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

#include <vector>
#include <Arduino.h>

namespace Networking
{
  //! Initializes the networking settings
  bool init();

  //! NOTE: This is insecure, only use this to open the landing and network pages from the hostname for the ESP32
  bool initAccessPoint();

  //! Scans available networks and adds their SSID to the result vector
  void scanAvailableNetworks(std::vector<String> &result);
  
  //! Starts the wifi connection
  void initWifiTimerConnect(TimerHandle_t timer);

  //! Disconnects the ESP32 access point and attempts to reconnect to the new network
  bool connectToNetwork(const String &Ssid, const String &Password);

  enum Status : unsigned int
  {
    ConnectedToAP = 0,
    ConnectedToWiFi,
    JoiningWiFi,
    NotConnected
  };
}

#endif
