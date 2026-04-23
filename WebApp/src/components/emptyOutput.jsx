/***************************************************************
 * File: analysisModule.jsx
 *
 * Date: 4/22/2026
 *
 * Description: UI component for an empty output.
 *
 * Author: Danielle Chang
 ***************************************************************/

import { moduleRegistry } from "../utils/defaultModules";

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
   * @brief Adds a new default module to the corresponding output
   */
  const handleAddModule = () => {
    // default to first module in module registry (major peaks)
    const newModule = {
      ...moduleRegistry[0],
      outputNumber: outputNum,
    };

    setModules((prev) => [...prev, newModule]);
  }

  return (
    <div className="pt-8 p-4 border border-blue-300 rounded-xl shadow-inner flex flex-col items-center">
      {/* Add module button */}
      <button
        className="p-3 cursor-pointer text-blue-400"
        onClick={handleAddModule}
      >
        +
      </button>
    </div>
  );
}
