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
import { useCallback, useEffect, useRef } from "preact/hooks";

/**
 * The frozen objects must match the 
 *  enumerations in storage.h under /main/,
 *  enumerations in utils.h under /main/,
 *  HTTP status codes in webInterface.cpp under /main/
 *  enumerations in Wave.h /src/ in the AudioLab respository
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
export const QUEUE_MESSAGE_ID = Object.freeze({
  EditGlobal: 0,
  EditModule: 1
});
export const CONFIG_FIELDS = Object.freeze({
  // Global
  "noiseFloor": 0,
  "cfarRefCount": 1,
  "cfarGuardCount": 2,
  "cfarBias": 3,
  "smoothingFactor": 4,

  // Shared module fields
  "minAmpNorm": 5,
  "freqLow": 6,
  "freqHigh": 7,
  "outputNumber": 8,

  // MajorPeaks
  "maxPeaks": 9,
  "frequencyMapping": 10,

  // Percussion
  "fluxThresh": 11,
  "energyThresh": 12,
  "entropyThresh": 13,
  "waveType": 14,

  // Shared module field
  "isMuted": 15
});
export const WAVE_TYPE = Object.freeze({
  0: "Sine",
  1: "Cosine",
  2: "Square",
  3: "Sawtooth",
  4: "Triangle"
});

// Internal
export const PAGE = {
  LANDING: 0,
  NETWORK: 1,
  MODULES: 2,
  RADIO: 3
};

/**
 * @brief The api util provides an generic interface for making API calls to the
 * backend web server
 *
 * @param {String} method - HTTP method to be used for the client request
 * @param {String} endpoint - API endpoint we want to invoke from the web server
 * @param {Object | null} data - Optional param for data
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
      case "PATCH": {
        return (await axios.patch(url, data));
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
  }
  catch (error) {
    console.error(`API Error [${method} ${url}]:`, error.message);
    return null;
  }
};

/**
 * @brief Hook for editing settings in real-time. Calls the API from the web server
 *        a max of 1/500ms when the setting is being changed to prevent flooding the web server
 * 
 * @param {Number} type - Message id to pass to the web server (type QUEUE_MESSAGE_ID)
 * @param {Object | null} isValid - Optional mutable reference to an isValid boolean 
 * 
 * @returns Hook for the edit setting callback
 */
export function useEditSetting(type, isValid = null) {
  const timers = useRef({});

  /**
   * @brief Function to be called on every update
   */
  const editSetting = useCallback( (setting) => {
    clearTimeout(timers.current[setting.id]);

    if (isValid?.current === false) return;

    // Only called 500ms after setting is settled
    timers.current[setting.id] = setTimeout( async () => {
      try {
        const payload = {
          ...setting,
          type
        };
        const res = await api("PATCH", "/analysis/editSetting", payload);

        if (res?.status == HTTP_STATUS.OK) {
          console.log("Success");
        }
      } 
      catch (error) {
        console.log(error);
      }
    }, 500);

  }, [type]);   // eslint-disable-line react-hooks/exhaustive-deps

  // Clean up the timers when in-use UI component is unmounted
  useEffect( () => {
    // Freeze object to avoid potential race conditions
    const currentTimers = timers.current;

    return () => {
      if (currentTimers) {
        Object.values(currentTimers).forEach(clearTimeout);
      }
    }
  }, []);

  return { editSetting };
}
