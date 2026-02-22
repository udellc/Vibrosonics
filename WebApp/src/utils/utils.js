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
 * The frozen objects must match the enumeration in storage.h under /main/
 */
export const HTTP_STATUS = Object.freeze({
  OK: 200,
  ACCEPTED: 202,
  BAD_REQUEST: 400,
  UNPROCESSABLE: 422,
  INTERNAL_ERROR: 500,
  UNAVAILABLE: 503
});
export const FREQUENCY_MAPPING = Object.freeze({
  0: "None",
  1: "Octave",
  2: "Midi"
});
export const MODULE_TYPE = Object.freeze({
  0: "Major Peaks",
  1: "Percussion"
});

/**
 * @brief The api util provides an generic interface for making API calls to the
 * backend web server
 *
 * @param method - HTTP method to be used for the client request
 * @param endpoint - API endpoint we want to invoke from the web server
 * @param data - Optional param for data
 *
 */
export const api = async (method, endpoint, data = null) => {
  const BASE_URL = "http://vibrosonics";
  const url = `${BASE_URL}${endpoint}`;

  try {
    switch (method) {
      case "GET": {
        return (await axios.get(url));
      }
      case "POST": {
        return (await axios.post(url, data));
      }
      case "PUT": {
        return (await axios.put(url, data));
      }
      case "DELETE": {
        return (await axios.delete(url));
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
