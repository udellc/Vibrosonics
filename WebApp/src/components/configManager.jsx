/***************************************************************
 * File: configManager.jsx
 *
 * Date: 02/26/2026
 *
 * Description: Component to manage whole configuration actions
 *
 * Author: Ivan Wong and Bella Mann
 ***************************************************************/

import { useContext, useState } from "preact/hooks";
import GLOBAL_PRESETS from "../data/globalSettingsData.json";
import { AudioSettingsContext } from "../utils/configurations";
import { moduleRegistry } from "../data/defaultModules";
import { api, HTTP_STATUS } from "../utils/utils";

const ConfigManager = ({ children }) => {
  const { globalSettings, setGlobalSettings } =
    useContext(AudioSettingsContext);
  const { modules, setModules } = useContext(AudioSettingsContext);


  const [currentProjectName, setCurrentProjectName] =useState("Project 1: Setup 1");
  const [activeGenre, setActiveGenre] = useState("Rock");
  const [library, setLibrary] = useState([]);
  const [projectCount, setProjectCount] = useState(1);
  const [setupCount, setSetupCount] = useState(1);

  const handlePresetClick = async (genre) => {
    setActiveGenre(genre);

    const preset = GLOBAL_PRESETS[genre];
    if(!preset) return;

    setGlobalSettings(preset.global);
    setModules(preset.modules);

    const payload = {
      global: preset.global,
      modules: preset.modules,
    };

    console.log("Sending preset:", genre);
    console.log("Payload:", JSON.stringify(payload, null, 2));
    
    const res = await api("PUT", "/analysis/submitSettings", payload);

    if (res?.status == HTTP_STATUS.OK){
      console.log("Preset applied:", genre);
    } else {
      console.log("Failed to apply preset", res?.status);
    }
  };

  const startNewProj = () => {
    if (window.confirm("Are you sure? Unsaved changes will be lost.")) {
      const nextCount = projectCount + 1;
      const initialSetup = 1;

      setProjectCount(nextCount);
      setSetupCount(initialSetup);
      setCurrentProjectName(`Project ${nextCount}: Setup ${initialSetup}`);

      // TODO: replaced initial states with this
    //   getSettings();

      // TODO: temp
      setActiveGenre("Rock");
    }
  };

  const saveProj = () => {
    // 1. Save the CURRENT name (e.g., "Project 2: Setup 1") to the library
    const newSave = {
      id: Date.now(),
      name: currentProjectName,
      data: { 
        globalSettings: structuredClone(globalSettings),
        modules: structuredClone(modules), 
        activeGenre 
      },
    };
    setLibrary((prev) => [...prev, newSave]);

    // 2. Prepare the name for the NEXT save within this same project
    const nextSetupNumber = setupCount + 1;
    setSetupCount(nextSetupNumber);
    setCurrentProjectName(`Project ${projectCount}: Setup ${nextSetupNumber}`);
  };

  const clearCurrentSettings = () => {
    setModules(structuredClone(moduleRegistry));
    setGlobalSettings(structuredClone(globalSettings));
    setActiveGenre("Rock");
  };

  const loadProject = (project) => {
    if (!project || !project.data) return;

    const { 
      globalSettings: savedGlobal,
      modules: savedModules,
      activeGenre: savedGenre 
    } = project.data;

    if (savedGlobal) setGlobalSettings(savedGlobal);
    if (savedModules) setModules(savedModules);
    if (savedGenre) setActiveGenre(savedGenre);

    setCurrentProjectName(project.name)
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

  return (
    <div>
      <h1 className="text-xl font-bold mb-4">{currentProjectName}</h1>
      <div className="flex flex-row flex-wrap items-center gap-8 p-4 pr-6 mt-8 mb-4 rounded-4xl border-gray-100 bg-gray-300 w-fit">
        <h2 className="text-xl font-bold">EQ Presets</h2>
        <div className="flex gap-2.5">
          {Object.keys(GLOBAL_PRESETS).map((genre) => (

            <button
              className={`py-1.5 px-8 rounded-lg cursor-pointer transition-colors
              ${
                activeGenre === genre
                  ? "bg-amber-200 font-bold border border-amber-600"
                  : "bg-[#ffffff] font-normal border border-gray-400"
              }`}
              key={genre}
              onClick={() => handlePresetClick(genre)}
            >
              {` ${genre} `}
            </button>
          ))}
        </div>
      </div>

      {/* Configuration stuff */}
      {children}

      <div className="p-8 mt-8 rounded-4xl bg-gray-300 mb-4 w-fit">
        <h2 className="text-2xl font-bold mb-6">Project Library</h2>

        <div className="flex gap-4 mb-6">
          <button
            onClick={saveProj}
            className="bg-[#70c247] text-white px-6 py-2 rounded-lg font-bold hover:bg-green-700 transition cursor-pointer border border-green-700"
          >
            Save Current Setup
          </button>
          <button
            onClick={startNewProj}
            className="bg-[#7face5] text-white px-6 py-2 rounded-lg font-bold hover:bg-blue-700 transition cursor-pointer border border-blue-700"
          >
            + Start New Project
          </button>
          <button
            onClick={clearLibrary}
            className="bg-[#ff6242] text-white px-6 py-2 rounded-lg hover:bg-red-700 font-bold cursor-pointer border border-red-700"
          >
            Clear All Projects
          </button>

          <button
            onClick={clearCurrentSettings}
            className="bg-[#ff9100] text-white px-6 py-2 rounded-lg font-bold hover:bg-orange-700 transition cursor-pointer border border-orange-700"
          >
            Clear Current Settings
          </button>
        </div>
      </div>

      {/* load project */}
      <div className="flex flex-col md:grid-cols-2 gap-4 mb-2">
        {library.map((project) => (
          <div
            key={project.id}
            className="p-4 rounded-4xl flex justify-between items-center bg-gray-300"
          >
            <span className="font-medium">{project.name}</span>
            <button
                onClick={() => loadProject(project)}
              className="text-sm font-bold text-blue-600 hover:underline"
            >
              Load Settings
            </button>
          </div>
        ))}
      </div>
    </div>
  );
};

export default ConfigManager;
