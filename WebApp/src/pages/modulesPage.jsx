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

const ModulesPage = () => {
  const { analysisSettings } = useContext(AudioSettingsContext);
  const [globalSettings, setGlobalSettings] = useState({});
  const [modules, setModules] = useState([]);

  /**
   * @brief
   */
  const getSettings = async () => {
    const res = await api("GET", "/analysis/getSettings");

    if (res.status == HTTP_STATUS.OK) {
      console.log(analysisSettings);
      setGlobalSettings(res.data.globalSettings);
      setModules(res.data.modules);
    }
  };

  useEffect( () => {
    // TODO: add API to get loaded settings
    getSettings();    
  }
  ,[]);

  return (
    <div>
      {/* Global settings here */}

      {modules.map((module) => {
        <AnalysisModule
          moduleParams={module}
        />
      })}
      <AnalysisModule />
    </div>
  );
};

export default ModulesPage;
