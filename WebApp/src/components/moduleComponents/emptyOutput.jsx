/***************************************************************
 * File: analysisModule.jsx
 *
 * Date: 4/22/2026
 *
 * Description: UI component for an empty output.
 *
 * Author: Danielle Chang
 ***************************************************************/

import { moduleRegistry } from "../../data/defaultModules";
import { HTTP_STATUS, MODULE_TYPE } from "../../utils/utils";
import { api } from "../../utils/utils.js";

/**
 * @brief The EmptyOutput component represents an output with no audio analysis modules.
 *
 * @param {Object} _ - Object representing the emtpy output channel
 * @param {Number} _.outputNum - Number of the output we are representing
 * @param {CallableFunction} _.setModules - Callback for updating the module settings and UI
 *
 * @returns AnalysisModule component describing the configs
 */
export default function EmptyOutput({ outputNum, setModules }) {
  
  /**
   * @brief Adds a new module to the corresponding output
   * 
   * @param {Number} value - Module type to be added
   */
  const handleAddModule = async (value) => {
    if (value === -1) {
      return;
    }
    const payload = {
      type: value,
      outputNumber: outputNum
    };
    const res = await api("POST", "/analysis/addModule", payload);

    if (res?.status === HTTP_STATUS.OK) {
      const newModule = {
        ...moduleRegistry[value],
        outputNumber: outputNum,
      };
      setModules((prev) => [...prev, newModule]);
    }
  };

  return (
    <div className="py-93 px-8 border-blue-300 rounded-xl shadow-inner flex flex-col">
      <div className="flex flex-row ">
        <select
          className="font-bold text-lg cursor-pointer appearance-none outline-none"
          value={-1}
          onChange={(e) => handleAddModule(e.target.value)}
        >
          <option key={-1} value={-1}>
            None
          </option>

          {Object.entries(MODULE_TYPE).map(([key, label]) => (
            <option key={key} value={key}>
              {label}
            </option>
          ))}
        </select>
        <div className="pointer-events-none inset-y-0 flex items-center">
          <span className="text-xs">▼</span>
        </div>
      </div>
    </div>
  );
}
