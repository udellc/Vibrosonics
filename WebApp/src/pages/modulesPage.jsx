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
import EmptyOutput from "../components/emptyOutput";

const ModulesPage = () => {
  // Persistant memory/data
  const { globalSettings, setGlobalSettings } = useContext(AudioSettingsContext);
  const { modules, setModules } = useContext(AudioSettingsContext);
  const [isExpertMode, setIsExpertMode] = useState(false);

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
  }, []);   // eslint-disable-line react-hooks/exhaustive-deps

  /**
   * @brief Gets the analysis config from the web server
   */
  const getSettings = async () => {
    const res = await api("GET", "/analysis/getSettings");

    if (res?.status == HTTP_STATUS.OK) {
      setGlobalSettings(res.data?.global ?? {});
      setModules(res.data?.modules ?? []);
    }
  };

  return (
    <div className="flex flex-col m-8">
      <ConfigManager>
        <div className="flex flex-row gap-4">
          {/* Either display empty slot or module assigned to each output
              We pass in module because the actual modules being sent to the server
              are updated here, rather than creating a copy of the module
          */}
          {outputs.map((module, outputNum) => {
            return (
              <div key={outputNum} className="gap-3 flex flex-col items-center">
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
        <GlobalSettings
          globalSettings={globalSettings}
          setGlobalSettings={setGlobalSettings}
          isExpertMode={isExpertMode} 
          setIsExpertMode={setIsExpertMode}
        />
          
      </ConfigManager>
    </div>
  );
};

export default ModulesPage;
