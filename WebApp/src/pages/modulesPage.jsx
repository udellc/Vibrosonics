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
import DropDown from "../atomics/dropdown";

const ModulesPage = () => {
  // Persistant memory/data
  const { globalSettings, setGlobalSettings } = useContext(AudioSettingsContext);
  const { modules, setModules } = useContext(AudioSettingsContext);
  const [tempType, setTempType] = useState('First'); {/** TODO: test functionality */}

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

  // TODO: actually test this function
  const handleAddModule = (moduleType) => {
    const newModule = {
      type: moduleType,
      id: Date.now(),
      outputNumber: selectedChannel,
      settings: {}
    };

    setModules(prev => [...prev, newModule]);

    api("POST", "/analysis/addModule", { module: newModule });
  }

  return (
    <div className="flex flex-col m-8">
      <ConfigManager>
        <>
          <div className="flex flex-col">
            <h4 className="font-bold text-lg">Output Channel:</h4>
            <DropDown label="Channel" options={['Major Peaks', 'Percussion']} onChange={(e) => handleDropdownChange(e)} ></DropDown>
          </div>

          <div className="flex flex-col pt-4">
            <h4 className="font-bold text-lg">Add New Module</h4>
            <div className="flex flex-row gap-x-4">
              <DropDown label="Module" options={['First', 'Second']} onChange={(e) => handleDropdownChange(e)} ></DropDown>
              <button className="rounded-lg pr-4 pl-4 bg-amber-200 font-bold border border-amber-600" onClick={() =>handleAddModule(tempType)} >Add Module</button>
            </div>
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
