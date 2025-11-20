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

namespace Networking
{
  //! Init Wi-Fi in access point mode
  //! NOTE: This is insecure, only use this to open the landing and network pages from the hostname for the ESP32
  bool initAccessPoint();

  //! 
  void scanAvailableNetworks();
  
  //!
  bool connectToNetwork();
  
  //!
  bool disconnectFromNetwork();
}

#endif
