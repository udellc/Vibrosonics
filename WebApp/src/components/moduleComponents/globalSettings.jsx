/***************************************************************
 * File: globalSettings.jsx
 *
 * Date: 2/21/2026
 *
 * Description: Displays the adjustable global audio config
 * settings.
 *
 * Author: Ivan Wong and Bella Mann
 ***************************************************************/

import Knob from "../../atomics/knob";
import { useState } from 'react';
import GlobalSettingsDisplay from "../../data/globalSettingsDisplay.json";
import { CONFIG_FIELDS, QUEUE_MESSAGE_ID, useEditSetting } from "../../utils/utils";

/**
 * @brief Displays the global configuration fields and updates them when changed
 * 
 * @param {Object} _ - Object containing required fields 
 * @param {Object} _.globalSettings - Global settings we want to display and modify 
 * @param {CallableFunction} _.setGlobalSettings - Callback that updates the global settings config and UI 
 * @param {any} [_.children]
 * @param {boolean} _.isExpertMode 
 * @param {CallableFunction} _.setIsExpertMode
 * @returns Global settings UI component
 */
const GlobalSettings = ({ globalSettings, setGlobalSettings, isExpertMode, setIsExpertMode}) => {
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
      value
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
    <div id="globalSettings">
      <button onClick={() => setIsCollapsed(!isCollapsed)} 
        className = "flex items-center gap-2 text-center justify-center font-bold text-xl mx-auto w-fit py-4">
        {GlobalSettingsDisplay.title}
        <span className="text-sm">{isCollapsed ? '▼' : '▲'}</span>
      </button>

      {!isCollapsed && (
      <div className="flex flex-col items-center pt-8 p-4 text-lg bg-gray-200 rounded-xl shadow-inner max-h-fit">
        <div className="flex items-center gap-3" id="mode">
          <span className={`text-sm font-medium tranistion-colors ${!isExpertMode ? 'text-gray-900' : 'text-gray-400'}`}>
            Beginner
          </span>

          <button
            type="button"
            className={`relative inline-flex h-6 w-11 flex-shrink-0 cursor-pointer rounded-full border-2 border-transparent transition-colors duration-200 ease-in-out focus:outline-none 
              ${isExpertMode ? 'bg-amber-500' : 'bg-gray-300'}`}
            role="switch" 
            aria-checked={isExpertMode}
            onClick={() => setIsExpertMode(!isExpertMode)}>
            <span
              aria-hidden="true"
              className={`pointer-events-none inline-block h-5 w-5 transform rounded-full bg-white shadow ring-0 transition duration-200 ease-in-out 
                ${isExpertMode ? 'translate-x-5' : 'translate-x-0'}`}/>
          </button>

          <span className={`text-sm font-bold transition-colors ${isExpertMode ? 'text-amber-600' : 'text-gray-400'}`}>
            Expert
          </span>
        </div>

        <div className="flex flex-wrap justify-center gap-4">
          {Object.entries(globalSettings)
          .filter(([key]) => isExpertMode || !settingsDisplay[key]?.isExpertOnly)
          .map(([key, val]) => {
            const displayTitle = (isExpertMode && settingsDisplay[key]?.expertTitle) ? settingsDisplay[key].expertTitle : settingsDisplay[key]?.title;
            const displayDescription = (isExpertMode && settingsDisplay[key]?.expertDescription) ? settingsDisplay[key].expertDescription  : settingsDisplay[key]?.description;
            
            return (
              <div key={key} className="flex flex-row">
                <Knob
                  id={`global-${key}`}
                  min={settingsDisplay[key].min}
                  max={settingsDisplay[key].max}
                  title={displayTitle}
                  step={settingsDisplay[key].step}
                  description={displayDescription}
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
