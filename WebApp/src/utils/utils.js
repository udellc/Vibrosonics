/***************************************************************
 * File: utils.js
 *
 * Date: 11/22/2025
 *
 * Description: Defines utility functions for the web app to use
 *
 * Author: Ivan Wong
 ***************************************************************/

import axios from "axios";

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
  const backendUrl = "http://vibrosonics-webapp";
  const url = `${backendUrl}${endpoint}`;

  try {
    switch (method) {
      case "GET": {
        return (await axios.get(url)).data;
      }
      case "POST": {
        return (await axios.post(url, data)).data;
      }
      case "PATCH": {
        return (await axios.patch(url, data)).data;
      }
      case "DELETE": {
        return (await axios.delete(url)).data;
      }
      default: {
        throw new Error(`Unknown HTTP method: ${method}`);
      }
    }
  } catch (error) {
    console.error(`API Error [${method} ${url}]:`, error.message);
    return null;
  }
};
