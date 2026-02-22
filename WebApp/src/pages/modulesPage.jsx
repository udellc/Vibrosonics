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

const ModulesPage = () => {
  const {globalSettings, setGlobalSettings} = useContext(AudioSettingsContext);
  const {modules, setModules} = useContext(AudioSettingsContext);

  /**
   * @brief Gets the analysis config from the web server
   */
  const getSettings = async () => {
    const res = await api("GET", "/analysis/getSettings");

    if (res.status == HTTP_STATUS.OK) {
      setGlobalSettings(res.data?.global);
      setModules(res.data?.modules);
    }
  };
  /**
   * @brief Gets the analysis configurations on mount
   */
  useEffect( () => {
    getSettings();
  }
  ,[]);

  return (
    <div className="flex flex-row m-8">
      <GlobalSettings 
        globalSettings={globalSettings}
        setGlobalSettings={setGlobalSettings}
      />
      {/* {modules.map((module) => {
        <AnalysisModule
          moduleParams={module}
        />
      })} */}
      {/* <AnalysisModule /> */}
    </div>
  );
};

export default ModulesPage;
