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
import GlobalSettings from "../components/globalSettings";
import ConfigManager from "../components/configManager";

const ModulesPage = () => {
  // Persistant memory/data
  const { globalSettings, setGlobalSettings } =
    useContext(AudioSettingsContext);
  const { modules, setModules } = useContext(AudioSettingsContext);

  // UI stuff
  const [selectedChannel, setSelectedChannel] = useState(0);
  const [displayedModules, setDisplayedModules] = useState([])

  const handleDropdownChange = (e) => {
    setSelectedChannel(e.target.value);
  };

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
  useEffect(() => {
    // Get filtered modules based on channel, preserving the index number for updates to the context
    const filtered = modules
      .map((m, index) => ({ ...m, index }))
      .filter(m => Number(m.outputNumber) === Number(selectedChannel));

    setDisplayedModules(filtered);
  }, [modules, selectedChannel]);

  /**
   * @brief Gets the analysis configurations on mount
   */
  useEffect(() => {
    getSettings();
  }, []);

  return (
    <div className="flex flex-col m-8">
      <ConfigManager>
        <>
          <div className="flex flex-col">
            <h4 className="font-bold text-lg">Output Channel:</h4>
            <select
              className="p-1 h-10 w-20 bg-blue-300 mb-4"
              onChange={(e) => handleDropdownChange(e)}
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
            {displayedModules?.map((m) => {
              return (
                <div key={m.index} className="gap-3">
                  <AnalysisModule
                    index={m.index}
                    module={modules[m.index]}
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
