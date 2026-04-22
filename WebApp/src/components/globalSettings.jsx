/***************************************************************
 * File: globalSettings.jsx
 *
 * Date: 2/21/2026
 *
 * Description: Displays the adjustable global audio config
 * settings.
 *
 * Author: Ivan Wong and Bella
 ***************************************************************/

import Knob from "../atomics/knob";
import { useState } from 'react';
import GlobalSettingsDisplay from "../data/globalSettingsDisplay.json";
import { CONFIG_FIELDS, QUEUE_MESSAGE_ID, useEditSetting } from "../utils/utils";

/**
 * @brief Displays the global configuration fields and updates them when changed
 * 
 * @param {Object} _ - Object containing required fields 
 * @param {Object} _.globalSettings - Global settings we want to display and modify 
 * @param {CallableFunction} _.setGlobalSettings - Callback that updates the global settings config and UI 
 * @param {any} [_.children]
 * @returns Global settings UI component
 */
const GlobalSettings = ({ globalSettings, setGlobalSettings, children }) => {
  const [isCollapsed, setIsCollapsed] = useState(true);

  const settingsDisplay = GlobalSettingsDisplay.settings;

  // Hook used for real-time updates
  const { editSetting } = useEditSetting(QUEUE_MESSAGE_ID.EditGlobal);
 
  /**
   * @brief Handles the knob display when value is changed
   *
   * @param {String} id -
   * @param {Number} value - Internal number in the Knob component
   */
  const handleKnobChange = (id, value) => {
    // Update the UI
    setGlobalSettings((prev) => ({ ...prev, [id]: value }));

    // Send a HTTP req for the modified setting
    editSetting({
      field: CONFIG_FIELDS[id],
      value: value
    });
  };

  // Check if the settings exists/ were retrieved
  // if (Object.keys(globalSettings).length == 0) {
  //   if (Object.keys(globalSettings).length === 0) {

  //   return (
  //     <div className="flex flex-col items-center justify-center p-10 m-4 bg-red-50 border border-red-200 rounded-xl shadow-sm">
  //       <h3 className="text-red-800 font-bold text-lg">Configuration Error</h3>
  //       <p className="text-red-600">Failed to get settings. Please check your connection.</p>
  //     </div>
  //   );}
  // }

  return (
    <div>
      <button onClick={() => setIsCollapsed(!isCollapsed)} 
        className = "flex items-center gap-2 text-center justify-center font-bold text-xl mx-auto w-fit py-4">
        {GlobalSettingsDisplay.title}
        <span className="text-sm">{isCollapsed ? '▼' : '▲'}</span>
      </button>

      {!isCollapsed && (
      <div className="flex flex-col items-center pt-8 p-4 text-lg bg-gray-200 rounded-xl shadow-inner max-h-fit">

        <div className="flex flex-wrap justify-center gap-4">
          {Object.entries(globalSettings).map(([key, val]) => {
            return (
              <div className="flex flex-row">
                {children}
                <Knob
                  min={settingsDisplay[key].min}
                  max={settingsDisplay[key].max}
                  title={settingsDisplay[key].title}
                  step={settingsDisplay[key].step}
                  description={settingsDisplay[key].description}
                  onChange={(value) => handleKnobChange(key, value)}
                  value={globalSettings[key] ?? val}
                />
              </div>
            );
          })}
        </div>
      </div>
      )}
    </div>
  );
};

export default GlobalSettings;
