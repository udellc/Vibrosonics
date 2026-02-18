/***************************************************************
 * File: configurations.js
 *
 * Date: 2/18/2026
 *
 * Description: Defines utility functions data management for
 * the web app.
 *
 * Author: Ivan Wong
 ***************************************************************/

import { useState } from "preact/hooks";
import { createContext } from "preact";

export const createProject = (name, libraryLength, currentValues) => {
  return {
    id: Date.now(),
    name: name || `Project ${libraryLength + 1}`,
    data: {
      knobValue: { ...currentValues.knobValue },
      sliderValue: { ...currentValues.sliderValue },
      isAdvanced: currentValues.isAdvanced,
      activeGenre: currentValues.activeGenre
    }
  };
};

// Persistant audio settings data
export const AudioSettingsContext = createContext(null);

/**
 * @brief Wrapper function that provides the children components
 * access to the audio settings data.
 * 
 * @param {Object} children - Children components
 */
export const AudioSettingsProvider = ({children}) => {
  const [analysisSettings, setAnalysisSettings] = useState(null);
  const data = {
    analysisSettings,
    setAnalysisSettings
  };
  return (
    <AudioSettingsContext.Provider value={data}>
      {children}
    </AudioSettingsContext.Provider>
  );
};
