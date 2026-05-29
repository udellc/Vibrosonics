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
import EQ_PRESETS from "../data/eqSettings.json";
import { AudioSettingsContext } from "../utils/configurations";
import { moduleRegistry } from "../data/defaultModules";
import defaultGlobalSettings from "../data/globalSettingsData.json";

const ConfigManager = ({ children }) => {
  const { globalSettings, setGlobalSettings } =
    useContext(AudioSettingsContext);
  const { modules, setModules } = useContext(AudioSettingsContext);


  const [currentProjectName, setCurrentProjectName] =useState("New Project");
  const [activeGenre, setActiveGenre] = useState("Rock");
  const [library, setLibrary] = useState([]);

  const startNewProj = () => {
    if (window.confirm("Are you sure? Unsaved changes will be lost.")) {
      setCurrentProjectName("New Project");

      // TODO: replaced initial states with this
    //   getSettings();

      // TODO: temp
      setActiveGenre("Rock");
    }
  };

  const saveProj = () => {
    const trimmedName = currentProjectName.trim();

    if(!trimmedName){
      alert("Please enter project name before saving!")
      return;
    }

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

  const loadProject = (project) => {
    if (!project || !project.data) return;

    const { 
      globalSettings: savedGlobal,
      modules: savedModules,
      activeGenre: savedGenre 
    } = project.data;

    if (savedGlobal) setGlobalSettings(structuredClone(savedGlobal));
    if (savedModules) setModules(structuredClone(savedModules));
    if (savedGenre) setActiveGenre(structuredClone(savedGenre));

    setCurrentProjectName(project.name)
  };

  const clearLibrary = () => {
    if (
      window.confirm(
        "This will permanently delete ALL saved projects. Continue?",
      )
    ) {
      setLibrary([]);
      setCurrentProjectName("New Project");
    }
  };

  return (
    <div>
      {/** Project name instertion box */}
      <div className="mb-4">
        <label htmlFor="project-name-input" className="block text-sm font-semibold text-gray-600 mb-1">
          Project Name
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
              onClick={() => setActiveGenre(genre)}
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
            Save Current Setup
          </button>
          <button
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
          </button>

          <button
            onClick={clearCurrentSettings}
            className="bg-[#ff9100] text-white px-6 py-2 rounded-lg font-bold hover:bg-orange-700 transition cursor-pointer border border-orange-700"
            id="clear-current-settings"
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
