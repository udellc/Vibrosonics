/***************************************************************
 * File: globalSettings.jsx
 *
 * Date: 2/21/2026
 *
 * Description: Displays the adjustable global audio config
 * settings.
 *
 * Author: Ivan Wong
 ***************************************************************/

import Knob from "../atomics/knob";
import GlobalSettingsDisplay from "../data/globalSettingsDisplay.json";

const GlobalSettings = ({ globalSettings, setGlobalSettings }) => {
  const settingsDisplay = GlobalSettingsDisplay.settings;
  /**
   * @brief Handles the knob display when value is changed
   *
   * @param {String} id -
   * @param {Number} value - Internal number in the Knob component
   */
  const handleKnobChange = (id, value) => {
    setGlobalSettings((prev) => ({ ...prev, [id]: value }));
  };

  if (Object.keys(globalSettings).length == 0) {
    // TODO: add UI component for failed to get settings
    return (
      <div>
        Error fetching analysis configs
      </div>
    );
  }
  return (
    <div className="flex flex-col items-center pt-8 p-4 text-lg bg-gray-200 rounded-xl shadow-inner max-h-fit">
      <h3 className="font-bold text-xl">{GlobalSettingsDisplay.title}</h3>

      <div className="grid grid-cols-2 gap-y-10">
        {Object.entries(globalSettings).map(([key, val]) => {
          return (
            <div>
              <Knob
                min={settingsDisplay[key].min}
                max={settingsDisplay[key].max}
                title={settingsDisplay[key].title}
                step={settingsDisplay[key].step}
                onChange={(value) => handleKnobChange(key, value)}
                value={globalSettings[key] ?? val}
              />
            </div>
          );
        })}
      </div>
    </div>
  );
};

export default GlobalSettings;
