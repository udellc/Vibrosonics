/***************************************************************
 * File: utils.js
 *
 * Date: 11/22/2025
 *
 * Description: Defines utility functions for the web app to use
 *
 * Author: Ivan Wong
 ***************************************************************/

/**
 * @brief The api util provides an generic interface for making API calls to the
 * backend web server
 *
 * @param method
 * @param endpoint
 * @param data
 *
 */
export const api = async (method, endpoint, data = null) => {
  // TODO: replace this with the actual backend url, this is the current default for ESP32 in AP mode -- has no internet
  const backendUrl = "http://192.168.4.1";
  const url = `${backendUrl}${endpoint};`;

  switch (method) {
    case "GET": {
      const res = await fetch(url);

      if (!res.ok) {
        // TODO: error handle
      }
      return res.json();
    }
    case "POST": {
      const res = await fetch(url, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(data),
      });
      if (!res.ok) {
        // TODO: error handle
      }
      return res.json();
    }
    default: {
        // TODO: add error handling for an unknow method
        return null;
    }
    // TODO: may need to add more cases for the PATCH and DELETE methods
  }
};
