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
import { FREQUENCY_MAPPING, MODULE_TYPE, useEditSetting, WAVE_TYPE, CONFIG_FIELDS, QUEUE_MESSAGE_ID, HTTP_STATUS } from "../utils/utils";
import { api } from "../utils/utils.js";
import { moduleRegistry } from "../data/defaultModules";

/**
 * @brief The AnalysisModule component describe a full module that can be modified
 * 
 * @param {Object} _ - Object describing the configurations
 * @param {Number} _.outputNum - Index of module to be updated using the modules context
 * @param {Object} _.module - Module configuration to modify
 * @param {CallableFunction} _.setModules - Callback for actually updating the module settings and UI
 * 
 * @returns AnalysisModule component describing the configs
 */
export default function AnalysisModule({ outputNum, module, setModules }) {
  
  /**
   * @todo Add better handling for invalid frequencies. currently just displays some red text if invalid
   * @brief Error handling invalid frequency ranges low and high
   */
  useEffect(() => {
    const freqLow = module?.freqLow;
    const freqHigh = module?.freqHigh;

    isValid.current = (freqLow <= freqHigh);

  }, [module?.freqLow, module?.freqHigh]);

  if (!module) return null;

  // eslint-disable-next-line react-hooks/rules-of-hooks
  const isValid = useRef(true);

  // eslint-disable-next-line react-hooks/rules-of-hooks
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
    setModules((prev) =>
      prev.map((m) => {
        if (m.outputNumber !== outputNum) return m;

        return {
          ...m,
          [id]: val,
        };
      })
    );

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
  const handleDeleteModule = async () => {
    if (!window.confirm("Are you sure you want to delete this module?")) {
      return;
    }
    const query = `index=${module.index}`;
    const res = await api("DELETE", `/analysis/deleteModule?${query}`);

    if (res?.status == HTTP_STATUS.OK) {
      setModules((prev) =>
        prev.filter((m) => m.outputNumber !== outputNum)
      );
    }
  };

  /**
   * @brief Updates the module type of the current output
   */
  const handleChangeModuleType = async (newType) => {
    newType = Number(newType);
    if (newType === -1) return;

    // delete current module
    const query = `index=${module.index}`;

    const deleteRes = await api("DELETE", `/analysis/deleteModule?${query}`);
    if (deleteRes?.status !== HTTP_STATUS.OK) return;

    // replace with default module of requested type
    const addRes = await api("POST", "/analysis/addModule", {
      type: newType,
      outputNumber: outputNum,
    });
    if (addRes?.status !== HTTP_STATUS.OK) return;

    // update UI
    const newModule = {
      ...moduleRegistry[newType],
      outputNumber: outputNum,
    };

    setModules((prev) =>
      prev.map((m) =>
        m.outputNumber === outputNum ? newModule : m
      )
    );
  };

  /**
   * @brief Mutes or unmutes current output
   */
  const handleMutePressed = () => {
    const updatedMuteVal = !module.isMuted;

    // Update the UI
    setModules((prev) =>
      prev.map((m) => {
        if (m.outputNumber !== outputNum) return m;

        return {
          ...m,
          isMuted: updatedMuteVal,
        };
      })
    );

    // Send the updated val to the web server
    editSetting({
      // This index is for the position in the web server array
      index: module.index,
      field: CONFIG_FIELDS["isMuted"],
      value: updatedMuteVal
    })
  }

  return (
    <div className="pt-8 p-4 bg-gray-200 rounded-xl shadow-inner flex flex-col items-center">
      <div className="flex flex-row">
        <button 
          className="text-black px-2 font-bold cursor-pointer"
          onClick={handleDeleteModule}
        >
          X
        </button>

        <div className="relative border border-black bg-white rounded-xl p-1.5">
        
        <select 
          className="w-full bg-transparent font-bold text-lg cursor-pointer appearance-none outline-none pr-6"
          value={module.moduleType}
          onChange={(e) => handleChangeModuleType(e.target.value)}
        >
          {Object.entries(MODULE_TYPE).map(([key, label]) => (
            <option key={key} value={key}>
              {label}
            </option>
          ))}
        </select>
        {/* Custom chevron icon since 'appearance-none' removes the default one */}
        <div className="pointer-events-none absolute inset-y-0 right-3 flex items-center">
          <span className="text-xs">▼</span>
        </div>
      </div>
      </div>

      {/* TODO: this is some logic saying ranges are not valid, add some sort of handling here */}
      <div>
        {isValid?.current ? (
          <div className="text-green-500"></div>
        ) : (
          <div className="font-bold text-red-500">outside valid ranges</div>
        )}
      </div>

      {/* Row layout */}
      <div className="flex flex-col gap-x-3 px-6">
        <button 
          className="w-fit self-center px-4 py-2 mt-2 text-black cursor-pointer rounded-xl bg-gray-300 hover:bg-gray-400 transition-colors"
          onClick={handleMutePressed}
        >
          {module.isMuted ? "Unmute" : "Mute"}
        </button>

        {/* Create a grid of knobs for corresponding settings */}
        <div className="grid grid-rows-3 grid-flow-col gap-5 py-2 px-4">
          {Object.entries(knobs)?.map( ([key, val]) => {
            return (
              <Knob
                key={key}
                min={val.min}
                max={val.max}
                step={val.step}
                onChange={(value) => handleValueChange(key, value)}
                title={val.title}
                description={val.description}
                value={module[key] ?? 0}
              />
            );
          })}
        </div>

        {/* Create dropdown boxes for corresponding settings */}
        <div className="flex flex-col gap-2">
          {Object.entries(dropdowns)?.map( ([key]) => {
            return (
              <div key={key} className="bg-white border rounded-xl px-2">
                <h4>{dropdowns[key].title}</h4>
                <select value={module[key] ?? 0}
                  onChange={(e) => handleValueChange(key, e.target instanceof HTMLSelectElement
                    ? Number(e.target.value)
                    : 1
                   )}>

                  {Object.entries(dropdownOptions)?.map( ([val, name]) => {
                    return <option key={name} value={val}>{name}</option>
                  })}

                </select>
              </div>
            );
          })}

          {/* Create numerical text entries for the corresponding settings */}
          {Object.entries(spinboxes)?.map( ([key, val]) => {
            return (
              <div key={key} className="bg-white border rounded-xl px-2">
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
