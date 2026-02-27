/***************************************************************
 * File: modulesPage.jsx
 *
 * Date: 11/22/2025
 *
 * Description: The audio analysis modules page for reconfiguring
 * haptic feedback
 *
 * Author: Ivan Wong
 ***************************************************************/

import { useContext, useEffect, useState } from "preact/hooks";
import AnalysisModule from "../components/analysisModule";
import { api, HTTP_STATUS } from "../utils/utils";
import { AudioSettingsContext } from "../utils/configurations";
import { createProject } from "../utils/configurations.js";
import GlobalSettings from "../components/globalSettings";
import Checkbox from "../atomics/checkbox";
import EQ_PRESETS from "../data/eqSettings.json";

// TODO: remove hwne done testing
import globalsEx from "../data/dataExample_global.json";
import modulesEx from "../data/dataExample_modules.json";
import ConfigManager from "../components/configManager";

const ModulesPage = () => {
  // Persistant memory/data
  // const { globalSettings, setGlobalSettings } =
  //   useContext(AudioSettingsContext);
  // const { modules, setModules } = useContext(AudioSettingsContext);

  // TODO: remove when done testing and uncomment ^^
  const [globalSettings, setGlobalSettings] = useState(globalsEx);
  const [modules, setModules] = useState(modulesEx);

  // UI stuff
  const [selectedChannel, setSelectedChannel] = useState(0);
  const [displayedModules, setDisplayedModules] = useState([]);

  const handleDropdownChange = (val) => {
    // TODO: implement
    console.log(val.target.value);
    setSelectedChannel(val);
  };

  /**
   * @brief Gets the analysis config from the web server
   */
  const getSettings = async () => {
    const res = await api("GET", "/analysis/getSettings");

    if (res.status == HTTP_STATUS.OK) {
      setGlobalSettings(res.data?.global || {});
      setModules(res.data?.modules || []);

      console.log(res.data?.global);
      console.log(res.data?.modules);
    }
  };
  /**
   * @brief Gets the analysis configurations on mount
   */
  useEffect(() => {
    getSettings();
  }, []);

  return (
    <div className="flex flex-col m-8">
      <ConfigManager>
        <div>
          <div className="flex flex-col">
            <h4 className="font-bold text-lg">Output Channel:</h4>
            <select
              className="p-1 h-10 w-20 bg-blue-300 mb-4"
              onChange={(val) => handleDropdownChange(val)}
            >
              <option value={0}>0</option>
              <option value={1}>1</option>
            </select>
          </div>
          <div className="flex flex-row gap-4">
            <GlobalSettings
              globalSettings={globalSettings}
              setGlobalSettings={setGlobalSettings}
            />
            {modules.map((m, index) => {
              return (
                <div className="gap-3">
                  <AnalysisModule
                    index={index}
                    module={m}
                    setModules={setModules}
                  />
                </div>
              );
            })}
          </div>
        </div>
      </ConfigManager>
    </div>
  );
};

export default ModulesPage;
