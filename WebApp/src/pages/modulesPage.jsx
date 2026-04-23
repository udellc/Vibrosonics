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
import InfoButton from "../atomics/infoButton";
import EmptyOutput from "../components/emptyOutput";

const ModulesPage = () => {
  // Persistant memory/data
  const { globalSettings, setGlobalSettings } = useContext(AudioSettingsContext);
  const { modules, setModules } = useContext(AudioSettingsContext);
  const [tempType, setTempType] = useState('First'); {/** TODO: test functionality */}

  // TODO: pull number of outputs from config rather than hard coding as 8
  const [outputs, setOutputs] = useState(new Array(8).fill(null));

  /**
   * @brief Updates output display whenever modules update.
   */
  useEffect(() => {
    let updatedOutputs = new Array(8).fill(null);
    for (const module of modules) {
      updatedOutputs[module.outputNumber] = module;
    }
    setOutputs(updatedOutputs);
  }, [modules]);

  /**
   * @brief Gets the analysis configurations on mount
   */
  useEffect(() => {
    getSettings();
  }, []);

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
          <div className="flex flex-col">
            <h4 className="font-bold text-lg">Output Channel:</h4>
            <select
              className="p-1 h-10 w-20 bg-white-300 mb-4 border rounded-xl"
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
                className="p-1 h-10 w-40 bg-white-300 mb-2 mr-4 border rounded-xl"
                onChange={(e) => handleAddDropdownChange(e)}
              >
                <option value={0}>Major Peaks</option>
                <option value={1}>Percussion</option>
              </select>
              <button
                className="bg-amber-200 cursor-pointer w-30 h-10 rounded-xl border border-amber-600 hover:bg-[#fbbf24]"
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

        {modules.length != 0 && (
          <div className="flex flex-row gap-4 pb-4">
            {outputs.map((module, outputNum) => {
              return (
                <div key={outputNum} className="gap-3">
                  <p>Output {outputNum + 1}</p>
                  {module === null ? (
                    <EmptyOutput
                      outputNum={outputNum}
                      setModules={setModules}
                    />
                  ) : (
                    <AnalysisModule
                      outputNum={outputNum}
                      module={module}
                      setModules={setModules}
                    />
                  )}
                </div>
              );
            })}
          </div>
        </>
        
        <GlobalSettings
            globalSettings={globalSettings}
            setGlobalSettings={setGlobalSettings}
          />
      </ConfigManager>
    </div>
  );
};

export default ModulesPage;
