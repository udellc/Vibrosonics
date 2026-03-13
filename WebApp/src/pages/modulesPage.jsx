/***************************************************************
 * File: modulesPage.jsx
 *
 * Date: 11/22/2025
 *
 * Description: The audio analysis modules page for reconfiguring
 * haptic feedback
 *
 * Author: Ivan Wong and Bella Mann
 ***************************************************************/

import { useContext, useEffect, useState } from "preact/hooks";
import AnalysisModule from "../components/analysisModule";
import { api, HTTP_STATUS } from "../utils/utils";
import { AudioSettingsContext } from "../utils/configurations";
import GlobalSettings from "../components/globalSettings";
import ConfigManager from "../components/configManager";
import { moduleRegistry } from "../utils/defaultModules";
import DropDown from "../atomics/dropdown";

const ModulesPage = () => {
  // Persistant memory/data
  const { globalSettings, setGlobalSettings } = useContext(AudioSettingsContext);
  const { modules, setModules } = useContext(AudioSettingsContext);
  const [tempType, setTempType] = useState('First'); {/** TODO: test functionality */}

  // UI stuff
  const [selectedChannel, setSelectedChannel] = useState(0);
  const [displayedModules, setDisplayedModules] = useState([]);
  const [moduleToAdd, setModuleToAdd] = useState(0);
  const [showAddModuleError, setShowAddModuleError] = useState(false);

  /**
   * @brief Updates the indices of the displayed modules according to the selected channel number
   */
  useEffect(() => {
    // Get filtered modules based on channel, saving the index for the modules context
    const displayedIndices = modules
      .map((m, index) =>
        Number(m.outputNumber) === Number(selectedChannel) ? index : -1,
      )
      .filter((index) => index !== -1);

    setDisplayedModules(displayedIndices);

  }, [modules, selectedChannel]);

  useEffect(() => {
    console.log(modules);
  }, modules);

  /**
   * @brief Gets the analysis configurations on mount
   */
  useEffect(() => {
    getSettings();
  }, []);

  /**
   * @brief Updates the selected channel for displayed modules
   * 
   * @param {*} e - Changed event for the channel dropdown component
   */
  const handleOutputDropdownChange = (e) => {
    setSelectedChannel(e.target.value);
    setShowAddModuleError(false);
  };

  /**
   * @brief Updates the selected module to add
   * 
   * @param {*} e - Changed event for the add module dropdown component
   */
  const handleAddDropdownChange = (e) => {
    setModuleToAdd(e.target.value);
    setShowAddModuleError(false);
  }

  /**
   * @brief Adds new module of a selected type to the module list
   */
  const handleAddModule = () => {
    // prevent user from adding duplicate modules to the same output
    if (modules.some(item => item["moduleType"] == moduleToAdd && item["outputNumber"] == selectedChannel)){
      setShowAddModuleError(true);
      return;
    }

    // add module from default registry assigned to current channel
    const newModule = { ...moduleRegistry[moduleToAdd] };
    newModule["outputNumber"] = selectedChannel;
    setModules([...modules, newModule]);

    setShowAddModuleError(false);
  }

  /**
   * @brief Gets the analysis config from the web server
   */
  const getSettings = async () => {
    const res = await api("GET", "/analysis/getSettings");

    if (res.status == HTTP_STATUS.OK) {
      setGlobalSettings(res.data?.global ?? {});
      setModules(res.data?.modules ?? []);
    }
  };

  return (
    <div className="flex flex-col m-8">
      <ConfigManager>
        <>
          <GlobalSettings
            globalSettings={globalSettings}
            setGlobalSettings={setGlobalSettings}
          />
          <div className="flex flex-col">
            <h4 className="font-bold text-lg">Output Channel:</h4>
            <select
              className="p-1 h-10 w-20 bg-blue-300 mb-4"
              onChange={(e) => handleOutputDropdownChange(e)}
            >
              <option value={0}>0</option>
              <option value={1}>1</option>
            </select>
          </div>
          <h4 className="font-bold text-lg">Add New Module:</h4>
          <div className="flex flex-col mb-4">
            <div className="flex">
              <select
                className="p-1 h-10 w-40 bg-blue-300 mb-2 mr-4"
                onChange={(e) => handleAddDropdownChange(e)}
              >
                <option value={0}>Major Peaks</option>
                <option value={1}>Percussion</option>
              </select>
              <button
                className="bg-amber-500 cursor-pointer w-30 h-10"
                onClick={handleAddModule}
              >
                Add Module
              </button>
            </div>
            {showAddModuleError && (
              <p className="text-red-500 text-sm">
                This module has already been added to channel {selectedChannel}
              </p>
            )}
          </div>

          <div className="flex flex-row gap-4">
            {/* Only display modules with the corresponding output channel number.
                We pass in modules[index] because the actual modules being sent to the server
                are updated here, rather than creating a copy of the module
            */}
            {displayedModules?.map((index) => {
              return (
                <div key={index} className="gap-3">
                  <AnalysisModule
                    index={index}
                    module={modules[index]}
                    setModules={setModules}
                  />
                </div>
              );
            })}
          </div>
        </>
      </ConfigManager>
    </div>
  );
};

export default ModulesPage;
