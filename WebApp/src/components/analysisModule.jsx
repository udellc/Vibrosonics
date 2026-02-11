/***************************************************************
 * File: analysisModule.jsx
 *
 * Date: 11/19/2025
 *
 * Description: UI component for an audio analysis module
 *
 * Author: Ivan Wong and Bella Mann
 ***************************************************************/

//import Slider from "../atomics/slider";
import Knob from "../atomics/knob";
import EQ_PRESETS from "../data/eqSettings.json";
import { useState } from "react";
import Checkbox from "../atomics/checkbox";
import PERCUSSION_PRESETS from "../data/precussionSettings.json";
import INITIAL_PRESETS from "../data/initalStates.json";
import { createProject } from "../utils/configurations.js";

// TODO: pass in a interface prop to define different knobs, sliders, etc.
export default function AnalysisModule() {
  const [activeGenre, setActiveGenre] = useState("Rock");
  const [knobValue, setKnobValue] = useState({});
  const [sliderValue, setSliderValue] = useState({});
  const [isAdvanced, setIsAdvanced] = useState(false);
  const [library, setLibrary] = useState([]);
  const [projectCount, setProjectCount] = useState(1);
  const [currentProjectName, setCurrentProjectName] = useState("Project 1");

  const currentModData = EQ_PRESETS[activeGenre];
  const modeKey = isAdvanced ? "Advanced Percussion" : "Percussion";
  const percussionData = PERCUSSION_PRESETS[modeKey];

  const handleKnobChange = (id, value) => {
    setKnobValue((prev) => ({ ...prev, [id]: value }));
    console.log(
      String("Knob") + String(id) + String("Value: ") + String(value)
    );
  };

  const handleSliderChange = (id, value) => {
    setSliderValue((prev) => ({ ...prev, [id]: value }));
    console.log(
      String("Slider") + String(id) + String("Value: ") + String(value)
    );
  };

  const handleValueChange = (id, value) => {
    const numValue = Number(value);

    const allPresets = [...percussionData, ...currentModData] // ADD MORE PRESETS HERE
    const preset = allPresets.find(p => p.id === id);
    const maxValue = preset ? preset.max : 100;
    const minValue = preset ? preset.min : 0;

    const clampedVal = Math.max(minValue, Math.min(maxValue, numValue));

    setKnobValue((prev) => ({ ...prev, [id]: clampedVal }));
  }

  const handleCheckboxChange = (value) => {
    setIsAdvanced(value);
  };

  const saveProject = (name) => {
    const newSave = createProject(name, library.length, {
      id: Date.now(),
      name: name || `Project ${library.length + 1}`,
        knobValue: {...knobValue},
        sliderValue: {...sliderValue},
        isAdvanced,
        activeGenre
      });
    setLibrary((prev) => [...prev, newSave]);
  };

  const startNewProj = () => {
    if(window.confirm("Are you sure? Unsaved changes will be lost.")) {
        const nextCount = projectCount + 1;
        setProjectCount(nextCount);
        setCurrentProjectName(`Project ${nextCount}`);

        setKnobValue({...INITIAL_PRESETS.knobs});
        setSliderValue(INITIAL_PRESETS.sliders);
        setIsAdvanced(INITIAL_PRESETS.isAdvanced);
        setActiveGenre(INITIAL_PRESETS.activeGenre);
    }
  };

  const clearCurrentSettings = () => {
    setKnobValue({...INITIAL_PRESETS.knobs});
    handleValueChange(0);
  }

  const loadProject = (project) => {
    if(!project || !project.data) return;

    const { knobValue, sliderValue, isAdvanced, activeGenre } = project.data;

    setKnobValue(knobValue);
    setSliderValue(sliderValue);
    setIsAdvanced(isAdvanced);
    setActiveGenre(activeGenre || 'Rock');
    setCurrentProjectName(project.name);
  }

  const clearLibrary = () => {
    if(window.confirm("This will permanently delete ALL saved projects. Continue?")) {
      setLibrary([]);
      setCurrentProjectName("Project 1");
    }
  };

  return (
    <div className="p-4">
      <h1 className="text-xl font-bold mb-4">{currentProjectName}</h1>
      
      <div className="flex flex-row flex-wrap items-start gap-8 p-8 mt-8 rounded-xl shadow-md border border-gray-100">
        {/*EQ Presets*/}
        <h2 className="text-xl font-bold mb-4">EQ</h2>
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
        <h2 className="text-xl font-bold mb-4">Percussion Settings
          <Checkbox
            label="Advanced Mode"
            onChange={(id, val) => handleCheckboxChange(val)}
          />
        </h2>
      </div>

      <div className="flex flex-row items-center items-stretch gap-8 mt-8 rounded-xl shadow-md border border-gray-100">
        {/*EQ Knobs and User Input*/}
        <div className="flex flex-row gap-5 p-5">
          <div className="flex flex-wrap gap-8 p-8 bg-gray-200 rounded-xl shadow-inner max-w-3xl">
              {currentModData.map((data) => (
                <div key={data.id} className="flex flex-col items-center gap-2">
                  <Knob
                    key={data.id}
                    title={data.title}
                    value={knobValue[data.id] ?? data.default ?? 0}
                    min={data.min}
                    max={data.max}
                    step={0.1}
                    onChange={(value) => handleKnobChange(data.id, value)}
                  />

                  <div className="flex flex-col items-center gap-1">
                    <input
                        type="number"
                        className="w-16 p-1 text-center bg-white border border-gray-400 rounded text-sm"
                        value={Math.round(knobValue[data.id] ?? data.initialValue ?? 0)}
                        onChange={(e) => handleValueChange(data.id, (e.target instanceof HTMLInputElement ? e.target.value : ""))}
                        min={data.min}
                        max={data.max}
                      />
                    </div>
                  <span className="font-bold text-gray-700">{data.label}</span>
                </div>
              ))}
          </div>
        </div>

        {/*Percussion Knobs and User Input*/}
          <div className={`${isAdvanced ? 'Advanced Percussion' : 'Percussion'} flex flex-row gap-5 p-5`}>
            <div className="flex flex-wrap gap-8 p-8 bg-gray-200 rounded-xl shadow-inner max-w-3xl">
                {percussionData.map((data) => (
                  <div key={data.id} className="flex flex-col items-center gap-2">
                    <Knob
                      key={data.id}
                      title={data.title}
                      value={knobValue[data.id] ?? data.initialValue ?? 0}
                      min={data.min}
                      max={data.max}
                      step={0.1}
                      onChange={(value) => handleKnobChange(data.id, value)}
                    />

                    <div className="flex flex-col items-center gap-1">
                      <input
                        type="number"
                        className="w-16 p-1 text-center bg-white border border-gray-400 rounded text-sm"
                        value={Math.round(knobValue[data.id] ?? data.initialValue ?? 0)}
                        onChange={(e) => handleValueChange(data.id, (e.target instanceof HTMLInputElement ? e.target.value : ""))}
                        min={data.min}
                        max={data.max}
                      />
                    </div>
                  </div>
                ))}
            </div>
          </div>
        </div>

      {/*Project Library Buttons*/}
      <div className="p-8 mt-8 bg-white rounded-xl shadow-md border border-gray-100">
        <h2 className="text-2xl font-bold mb-6">Project Library</h2>
        
        <div className="flex gap-4 mb-6">
          <button 
            onClick={() => saveProject()} 
            className="bg-green-600 text-white px-6 py-2 rounded-lg font-bold hover:bg-green-700 transition"
          >
            Save Current Setup
          </button>
          <button 
            onClick={startNewProj} 
            className="bg-blue-600 text-white px-6 py-2 rounded-lg font-bold hover:bg-blue-700 transition"
          >
            + Start New Project
          </button>
          <button onClick={clearLibrary} className="bg-red-600 text-white px-6 py-2 rounded-lg font-bold">
            Clear All Projects
          </button>

          <button 
            onClick={clearCurrentSettings} 
            className="bg-orange-600 text-white px-6 py-2 rounded-lg font-bold hover:bg-orange-700 transition"
          >
            Clear Current Settings
          </button>
        </div>

        {/*Load Settings Button*/}
        <div className="flex flex-col md:grid-cols-2 gap-4">
          {library.map((project) => (
            <div key={project.id} className="p-4 border rounded-lg flex justify-between items-center bg-gray-50">
              <span className="font-medium">{project.name}</span>
              <button onClick={() => {loadProject(project)}} className="text-sm text-blue-600 hover:underline">
                Load Settings
              </button>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
