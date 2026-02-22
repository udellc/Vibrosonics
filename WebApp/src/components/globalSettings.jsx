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
import AnalysisSettingsDisplay from "../data/analysisSettingsDisplay.json";

const GlobalSettings = ({ globalSettings, setGlobalSettings }) => {
  const settingsDisplay = AnalysisSettingsDisplay.global.settings;

  /**
   * @brief Handles the knob display when value is changed
   *
   * @param {String} id -
   * @param {Number} value - Internal number in the Knob component
   */
  const handleKnobChange = (id, value) => {
    setGlobalSettings((prev) => ({ ...prev, [id]: value }));
  };

  /**
   * @brief Handles the text input display and min/max boundaries
   * @param {String} id - ID of the setting
   * @param {Number} value - Value of the setting
   */
  const handleValueChange = (id, value) => {
    const numValue = Number(value);
    const maxValue = settingsDisplay[id].max;
    const minValue = settingsDisplay[id].min;
    const clampedVal = Math.max(minValue, Math.min(maxValue, numValue));

    setGlobalSettings((prev) => ({ ...prev, [id]: clampedVal }));
  };
  return (
    <div className="flex flex-col items-center p-8 text-xl bg-gray-200 rounded-xl shadow-inner max-h-fit">
      <h1 className="font-bold text-2xl">Global Configurations</h1>

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
                value={Number(parseFloat(globalSettings[key] ?? val).toFixed(2))}
              />
              <div className="flex flex-col items-center gap-1">
                <input
                  type="number"
                  className="w-16 p-1 text-center bg-white border border-gray-400 rounded text-sm"
                  value={Number(parseFloat(globalSettings[key] ?? val).toFixed(2))}
                  onChange={(e) =>
                    handleValueChange(
                      key,
                      e.target instanceof HTMLInputElement
                        ? Number(e.target.value)
                        : 0,
                    )
                  }
                  min={settingsDisplay[key].min}
                  max={settingsDisplay[key].max}
                />
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
};

export default GlobalSettings;
