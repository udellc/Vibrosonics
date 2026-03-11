/***************************************************************
 * File: analysisModule.jsx
 *
 * Date: 11/19/2025
 *
 * Description: UI component for an audio analysis module
 *
 * Author: Ivan Wong and Bella Mann
 ***************************************************************/

import { useEffect, useRef } from "preact/hooks";
import Knob from "../atomics/knob";
import ModuleDisplay from "../data/moduleDisplay.json";
import { FREQUENCY_MAPPING, MODULE_TYPE, useEditSetting, WAVE_TYPE, CONFIG_FIELDS, QUEUE_MESSAGE_ID } from "../utils/utils";

/**
 * @brief The AnalysisModule component describe a full module that can be modified
 * 
 * @param {Object} _ - Object describing the configurations
 * @param {Number} _.index - Index of module to be updated using the modules context
 * @param {Object} _.module - Module configuration to modify
 * @param {CallableFunction} _.setModules - Callback for actually updating the module settings and UI
 * 
 * @returns AnalysisModule component describing the configs
 */
export default function AnalysisModule({ index, module, setModules }) {
  if (!module) return null;

  const isValid = useRef(true);
  const { editSetting } = useEditSetting(QUEUE_MESSAGE_ID.EditModule, isValid);

  // Getting the module specific settings display values and ranges from the /data/ directory
  const settings = ModuleDisplay[module.moduleType].settings;
  const knobs = settings.knob ?? null;
  const dropdowns = settings.dropdown ?? null;
  const spinboxes = settings.spinbox ?? null;

  // FIXME: temp solution to dynamic dropdown options
  const getDropdownOptions = () => {
    // Major peaks
    if (module.moduleType === 0) return FREQUENCY_MAPPING;

    // Percussion
    else if (module.moduleType === 1) return WAVE_TYPE;
  }
  const dropdownOptions = getDropdownOptions();

  /**
   * @brief Updates the module field in the UI and module context
   * 
   * @param {String} id - The field we want to be updated
   * @param {any} val - New value we want to set
   */
  const handleValueChange = (id, val) => {
    // Update the UI
    setModules((prev) => {
      const updated = [...prev];

      // This index is for the UI and may not be same for the web server
      updated[index] = {
        ...updated[index],
        [id]: val,
      };
      return updated;
    });
    // Send the updated val to the web server
    editSetting({
      // This index is for the position in the web server array
      index: module.index,
      field: CONFIG_FIELDS[id],
      value: val
    })
  };

  /**
   * @brief Deletes the current module from the module list
   */
  const handleDeleteModule = () => {
    setModules((prev) => {
      const updated = [...prev];
      updated.splice(index, 1); 
      return updated;
    });
  };

  /**
   * @todo Add better handling for invalid frequencies. currently just displays some red text if invalid
   * @brief Error handling invalid frequency ranges low and high
   */
  useEffect(() => {
    const freqLow = module.freqLow;
    const freqHigh = module.freqHigh;

    isValid.current = (freqLow <= freqHigh);

  }, [module.freqLow, module.freqHigh]);

  return (
    <div className="pt-8 p-4 bg-gray-200 rounded-xl shadow-inner flex flex-col items-center">
      <button 
        className="bg-amber-500 cursor-pointer"
        onClick={handleDeleteModule}
      >
        Delete
      </button>
      <h3 className="font-bold text-lg">
        Module: {MODULE_TYPE[module.moduleType]}
      </h3>

      {/* TODO: this is some logic saying ranges are not valid, add some sort of handling here */}
      <div>
        {isValid?.current ? (
          <div>Valid ranges</div>
        ) : (
          <div className="font-bold text-red-500">Not valid ranges</div>
        )}
      </div>

      {/* Row layout */}
      <div className="flex flex-row gap-x-3">

        {/* Create a grid of knobs for corresponding settings */}
        <div className="grid grid-rows-3 grid-flow-col gap-5">
          {Object.entries(knobs)?.map( ([key, val]) => {
            return (
              <Knob 
                min={val.min}
                max={val.max}
                step={val.step}
                onChange={(value) => handleValueChange(key, value)}
                title={val.title}
                value={module[key] ?? 0}
              />
            );
          })}
        </div>

        {/* Create dropdown boxes for corresponding settings */}
        <div className="flex flex-col gap-2">
          {Object.entries(dropdowns)?.map( ([key, _]) => {
            return (
              <div className="bg-blue-300">
                <h4>{dropdowns[key].title}</h4>
                <select value={module[key] ?? 0}
                  onChange={(e) => handleValueChange(key, e.target instanceof HTMLSelectElement
                    ? Number(e.target.value)
                    : 1
                   )}>

                  {Object.entries(dropdownOptions)?.map( ([val, name]) => {
                    return <option value={val}>{name}</option>
                  })}

                </select>
              </div>
            );
          })}

          {/* Create numerical text entries for the corresponding settings */}
          {Object.entries(spinboxes)?.map( ([key, val]) => {
            return (
              <div className="bg-blue-300">
                <h4>{spinboxes[key].title}</h4>
                <input type="number" 
                  value={module[key] ?? 1}
                  min={val.min}
                  max={val.max}
                  onChange={(e) => handleValueChange(key,
                    e.target instanceof HTMLInputElement
                      ? Number(e.target.value)
                      : 1
                  )}
                />
              </div>
            );
          })}
        </div>
      </div>
    </div>
  );
}
