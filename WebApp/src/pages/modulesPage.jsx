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
    // const res = await api("GET", "/audio/getSettings");

    // if (res.status == HTTP_STATUS.OK) {
      // console.log(analysisSettings);
      // setGlobalSettings(res.data.globalSettings);
      // setModules(res.data.modules);
    // }
  };

  useEffect( () => {
    // TODO: add API to get loaded settings
    getSettings();    
  }
  ,[]);

  return (
    <div>
      <h1 className="font-bold mt-10">
        <AnalysisModule />
      </h1>
    </div>
  );
};

export default ModulesPage;
