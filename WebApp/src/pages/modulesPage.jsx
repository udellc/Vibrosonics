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
  const [displayedModules, setDisplayedModules] = useState([]);

  /**
   * @brief Updates the selected channel for displayed modules
   * 
   * @param {*} e - Changed event for the channel dropdown component
   */
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

    // FIXME: the length may not be a good indicator, but this works for now
  }, [modules.length, selectedChannel]);

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
