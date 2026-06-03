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
import EQ_PRESETS from "../../data/eqSettings.json";
import { AudioSettingsContext } from "../../utils/configurations";
import { moduleRegistry } from "../../data/defaultModules";
import defaultGlobalSettings from "../../data/globalSettingsData.json";
import { api, HTTP_STATUS } from "../../utils/utils";

/**
 * @brief Parent container for the modules page
 * 
 * @param {Object} _ - Expanded object
 * @param {*} _.children - Components within the container
 *  
 * @returns 
 */
const ConfigManager = ({ children }) => {
  const { globalSettings, setGlobalSettings } =   //eslint-disable-line no-unused-vars
    useContext(AudioSettingsContext);
  const { modules, setModules } = useContext(AudioSettingsContext);   //eslint-disable-line no-unused-vars

  const [currentProjectName, setCurrentProjectName] =useState("New Project");
  const [activeGenre, setActiveGenre] = useState("Rock");
  const [library, setLibrary] = useState([]);

  const saveProj = async () => {
    const trimmedName = currentProjectName.trim();

    if(!trimmedName){
      alert("Please enter project name before saving!")
      return;
    }
    const res = await api("POST", "/analysis/saveSettings", { name: trimmedName });

    if (res?.status === HTTP_STATUS.OK) {
      const newProject = {
        id: Date.now(),             // For UI mapping
        name: currentProjectName    // Only send the name, the current settings will be saved in the web server
      }
      setLibrary((prev) => [...prev, newProject])
    }
  };

  const clearCurrentSettings = () => {
    if (Array.isArray(moduleRegistry)) {
        setModules([...moduleRegistry]);
    } else if (moduleRegistry && typeof moduleRegistry === 'object') {
        setModules(Object.values(moduleRegistry));
    }
    setGlobalSettings({...defaultGlobalSettings.global});
    setActiveGenre("Rock");
  };

  const loadProject = async (name) => {
    if (!name) return;

    const res = await api("GET", "/analysis/getPreset", {name});

    if (res?.status === HTTP_STATUS.OK) {
      const project = res?.data;

      setGlobalSettings(project.global);
      setModules(project.modules);
      setCurrentProjectName(project.name);
    }

  };

  const clearLibrary = () => {
    if (
      window.confirm(
        "This will permanently delete ALL saved presets. Continue?",
      )
    ) {
      setLibrary([]);
      setCurrentProjectName("New Project");
    }
  };

  const handlePresetClick = async (genre) => {
    setActiveGenre(genre);

    const preset = EQ_PRESETS[genre];
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

  return (
    <div>
      {/** Project name instertion box */}
      <div className="mb-4">
        <label htmlFor="project-name-input" className="block text-sm font-semibold text-gray-600 mb-1">
          Current Settings
          </label>
          <input
            id="project-name-input"
            type="text"
            value={currentProjectName}
            onChange={(e) => setCurrentProjectName(e.target?.value)}
            placeholder="Type project name here..."
            className="text-xl font-bold bg-transparent border-b-2 border-gray-400 focus:border-blue-500 outline-none pb-1 w-full max-w-md transition-colors"
          />
      </div>

      <div className="flex flex-row flex-wrap items-center gap-8 p-4 pr-6 mt-8 mb-4 rounded-4xl border-gray-100 bg-gray-300 w-fit">
        <h2 id="modules-button" className="text-xl font-bold">EQ Presets</h2>
        <div className="flex gap-2.5">
          {Object.keys(EQ_PRESETS).map((genre) => (
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

      <div className="p-8 mt-8 rounded-4xl bg-gray-300 mb-4 w-fit" id="library">
        <h2 className="text-2xl font-bold mb-6">Project Library</h2>

        <div className="flex gap-4 mb-6">
          <button
            onClick={saveProj}
            className="bg-[#70c247] text-white px-6 py-2 rounded-lg font-bold hover:bg-green-700 transition cursor-pointer border border-green-700"
            id="current-setup"
          >
            Save Current Settings
          </button>
          <button
            onClick={clearCurrentSettings}
            className="bg-[#ff9100] text-white px-6 py-2 rounded-lg font-bold hover:bg-orange-700 transition cursor-pointer border border-orange-700"
            id="clear-current-settings"
          >
            Reset Settings
          </button>
          <button
            onClick={clearLibrary}
            className="bg-[#ff6242] text-white px-6 py-2 rounded-lg hover:bg-red-700 font-bold cursor-pointer border border-red-700"
            id="clear-all"
          >
            Clear All Projects
          </button>
          {/* <button
            onClick={startNewProj}
            className="bg-[#7face5] text-white px-6 py-2 rounded-lg font-bold hover:bg-blue-700 transition cursor-pointer border border-blue-700"
            id="new-project"
          >
            + Start New Project
          </button>
          <button
            onClick={clearLibrary}
            className="bg-[#ff6242] text-white px-6 py-2 rounded-lg hover:bg-red-700 font-bold cursor-pointer border border-red-700"
            id="clear-all"
          >
            Clear All Projects
          </button> */}
        </div>
      </div>
      <div className="flex flex-col md:grid-cols-2 gap-4 mb-2">
        {library.map((project) => (
          <div
            key={project.id}
            className="p-4 rounded-4xl flex justify-between items-center bg-gray-300"
          >
            <span className="font-medium">{project.name}</span>
            <button
                onClick={() => loadProject(project)}
              className="text-sm font-bold text-blue-600 hover:underline cursor-pointer"
            >
              Load Preset
            </button>
            <button
                onClick={() => loadProject(project)}
              className="text-sm font-bold text-blue-600 hover:underline cursor-pointer"
            >
              Delete Preset
            </button>
          </div>
        ))}
      </div>
    </div>
  );
};

export default ConfigManager;
