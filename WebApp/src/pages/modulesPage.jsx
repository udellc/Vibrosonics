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

const ModulesPage = () => {
  // Persistant memory/data
  const { globalSettings, setGlobalSettings } = useContext(AudioSettingsContext);
  const { modules, setModules } = useContext(AudioSettingsContext);

  // Project management
  const [currentProjectName, setCurrentProjectName] = useState("Project 1: Setup 1");
  const [activeGenre, setActiveGenre] = useState("Rock");
  const [isAdvanced, setIsAdvanced] = useState(false);
  const [library, setLibrary] = useState([]);
  const [projectCount, setProjectCount] = useState(1);
  const [setupCount, setSetupCount] = useState(1);
  const handleCheckboxChange = (value) => setIsAdvanced(value);

  const saveProject = (name) => {
    const newSave = createProject(name, library.length, {
      id: Date.now(),
      name: name || `Project ${library.length + 1}`,
      knobValue: { ...globalSettings },
      isAdvanced,
      activeGenre,
    });
    console.log("Save project herererer!!!");
    console.log(globalSettings);
    setLibrary((prev) => [...prev, newSave]);
  };

  const startNewProj = () => {
    if (window.confirm("Are you sure? Unsaved changes will be lost.")) {
      const nextCount = projectCount + 1;
      const initialSetup = 1;

      setProjectCount(nextCount);
      setSetupCount(initialSetup);
      setCurrentProjectName(`Project ${nextCount}: Setup ${initialSetup}`);
      setIsAdvanced(false);

      // TODO: replaced initial states with this
      getSettings();

      // TODO: temp
      setActiveGenre("Rock");
    }
  };

  const saveProj = () => {
    // 1. Save the CURRENT name (e.g., "Project 2: Setup 1") to the library
    const newSave = {
      id: Date.now(),
      name: currentProjectName,
      data: { globalSettings, isAdvanced, activeGenre },
    };
    setLibrary((prev) => [...prev, newSave]);

    // 2. Prepare the name for the NEXT save within this same project
    const nextSetupNumber = setupCount + 1;
    setSetupCount(nextSetupNumber);
    setCurrentProjectName(`Project ${projectCount}: Setup ${nextSetupNumber}`);
  };

  const clearCurrentSettings = () => {
    getSettings();
    setIsAdvanced(false);
    setActiveGenre("Rock");
  };

  const loadProject = (project) => {
    if (!project || !project.data) return;

    const { knobValue, isAdvanced, activeGenre } = project.data;

    // setKnobValue(knobValue);
    setIsAdvanced(isAdvanced);
    setActiveGenre(activeGenre || "Rock");
    setCurrentProjectName(project.name);
  };

  const clearLibrary = () => {
    if (
      window.confirm(
        "This will permanently delete ALL saved projects. Continue?",
      )
    ) {
      setLibrary([]);
      setProjectCount(1);
      setSetupCount(1);
      setCurrentProjectName("Project 1: Setup 1");
    }
  };

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
  useEffect(() => {
    getSettings();
  }, []);

  /**
   * @brief Sends the global settings and modules to the web server
   */
  const sendData = async () => {
    const payload = {
      global: globalSettings,
      modules
    }
    const res = await api("PUT", "/analysis/submitSettings", payload);

    if (res.status == HTTP_STATUS.OK) {
      console.log("worked");
    }
    else
      console.log(res.status);
  };
  return (
    <div className="flex flex-col m-8">
      <h1 className="text-xl font-bold mb-4">{currentProjectName}</h1>

      {/* preset stuff */}
      <div className="flex flex-row flex-wrap items-start gap-8 p-8 mt-8 mb-8 rounded-xl shadow-md border border-gray-100">
        <h2 className="text-xl font-bold mb-4">Presets</h2>
        <div className="flex gap-2.5 mb-8">
          {Object.keys(EQ_PRESETS).map((genre) => (
            <button
              className={`p-3 border border-[#ccc] rounded-lg cursor-pointer transition-colors
              ${
                activeGenre === genre
                  ? "bg-[#fcd34d] font-bold"
                  : "bg-[#e5e7eb] font-normal"
              }`}
              key={genre}
              onClick={() => setActiveGenre(genre)}
            >
              {` ${genre} `}
            </button>
          ))}
        </div>

        {/*Precussion Presets*/}
        <h2 className="text-xl font-bold mb-4">
          Percussion Settings
          <Checkbox
            label="Advanced Mode"
            onChange={(val) => handleCheckboxChange(val)}
          />
        </h2>
        {/* TODO: testing this out rq */}
        <div>
          <button className="bg-amber-500 cursor-pointer"
          onClick={sendData}>
            SEND ITTTTTTTTTTT
          </button>
          <button className="ml-6 bg-amber-500 cursor-pointer"
          onClick={() => {
            console.log(modules);
            console.log(globalSettings);
          }}>
            Printint
          </button>
        </div>
      </div>

      {/* analysis settings stuff */}
      <div className="flex flex-row gap-4">
        <GlobalSettings
          globalSettings={globalSettings}
          setGlobalSettings={setGlobalSettings}
        />
        {modules.map((module, index) => {
          return (
            <div className="gap-3">
              <AnalysisModule
                channel={index}
                module={module}
                setModules={setModules}
              />
            </div>
          );
        })}
      </div>

      {/* project management stuff */}
      <div className="p-8 mt-8 bg-white rounded-xl shadow-md border border-gray-100">
        <h2 className="text-2xl font-bold mb-6">Project Library</h2>

        <div className="flex gap-4 mb-6">
          <button
            onClick={saveProj}
            className="bg-green-600 text-white px-6 py-2 rounded-lg font-bold hover:bg-green-800 transition cursor-pointer"
          >
            Save Current Setup
          </button>
          <button
            onClick={startNewProj}
            className="bg-blue-600 text-white px-6 py-2 rounded-lg font-bold hover:bg-blue-800 transition cursor-pointer"
          >
            + Start New Project
          </button>
          <button
            onClick={clearLibrary}
            className="bg-red-600 text-white px-6 py-2 rounded-lg hover:bg-red-800 font-bold cursor-pointer"
          >
            Clear All Projects
          </button>

          <button
            onClick={clearCurrentSettings}
            className="bg-orange-600 text-white px-6 py-2 rounded-lg font-bold hover:bg-orange-800 transition cursor-pointer"
          >
            Clear Current Settings
          </button>
        </div>
      </div>

      {/* load project */}
      <div className="flex flex-col md:grid-cols-2 gap-4">
        {library.map((project) => (
          <div
            key={project.id}
            className="p-4 border rounded-lg flex justify-between items-center bg-gray-50"
          >
            <span className="font-medium">{project.name}</span>
            <button
              onClick={() => loadProject(project)}
              className="text-sm text-blue-600 hover:underline"
            >
              Load Settings
            </button>
          </div>
        ))}
      </div>
    </div>
  );
};

export default ModulesPage;
