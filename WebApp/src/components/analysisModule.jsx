/***************************************************************
 * File: analysisModule.jsx
 *
 * Date: 11/19/2025
 *
 * Description: UI component for an audio analysis module
 *
 * Author: Ivan Wong and Bella Mann
 ***************************************************************/

import Slider from "../atomics/slider";
import Knob from "../atomics/knob";
import EQ_PRESETS from "../data/eqSettings.json";
import { useState } from "react";
import Checkbox from "../atomics/checkbox";
import PERCUSSION_PRESETS from "../data/precussionSettings.json";

// TODO: pass in a interface prop to define different knobs, sliders, etc.
export default function AnalysisModule() {
  const [activeGenre, setActiveGenre] = useState("Rock");
  const [knobValue, setKnobValue] = useState({});
  const [sliderValue, setSliderValue] = useState({});
  const [isAdvanced, setIsAdvanced] = useState(false);

  const currentSliders = EQ_PRESETS[activeGenre];
  const currentKnobs = EQ_PRESETS[activeGenre];

  const modeKey = isAdvanced ? "Advanced Percussion" : "Percussion";
  const percussionData = PERCUSSION_PRESETS[modeKey];
  const percussionKnobs = PERCUSSION_PRESETS[modeKey];

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

  const handleCheckboxChange = (id, value) => {
    setIsAdvanced(value);
  };

  return (
    <div className="p-4">
      <h1 className="text-xl font-bold mb-4">EQ</h1>

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

      <div className="flex flex-wrap gap-8 p-8 bg-gray-200 rounded-xl shadow-inner max-w-3xl">
          {currentKnobs.map((knobData) => (
            <div key={knobData.id} className="flex flex-col items-center gap-2">
              <Knob
                key={knobData.id}
                title={knobData.title}
                value={knobValue[knobData.id] ?? knobData.default ?? 0}
                min={knobData.min}
                max={knobData.max}
                step={0.1}
                onChange={(value) => handleKnobChange(knobData.id, value)}
              />
              <span className="font-bold text-gray-700">{knobData.label}</span>
            </div>
          ))}

        <div className="flex flex-row gap-5 p-5">
          {currentSliders.map((slider) => (
            <Slider
              key={slider.id}
              title={slider.title}
              initialValue={sliderValue[slider.id] ?? slider.default ?? 0}
              min={slider.min}
              max={slider.max}
              step={2}
              onInput={(sliderValue) =>
                handleSliderChange(slider.id, sliderValue)
              }
            />
          ))}
        </div>
      </div>

      <div className="p-4">
        <h2 className="text-xl font-bold mb-4">Percussion Settings
          <Checkbox
            label="Advanced Mode"
            onChange={(id, val) => handleCheckboxChange(id, val)}
          />
        </h2>

        <div className={`${isAdvanced ? 'Advanced Percussion' : 'Percussion'} flex flex-row gap-5 p-5`}>
          <div className="flex flex-wrap gap-8 p-8 bg-gray-200 rounded-xl shadow-inner max-w-3xl">
              {percussionData.map((data) => (
                <div key={data.id} className="flex flex-col items-center gap-8">
                  <Knob
                    key={data.id}
                    title={data.title}
                    value={knobValue[data.id] ?? data.initialValue ?? 0}
                    min={data.min}
                    max={data.max}
                    step={0.1}
                    onChange={(value) => handleKnobChange(data.id, value)}
                  />

                  <Slider
                    key={data.id}
                    title={data.title}
                    initialValue={sliderValue[data.id] ?? data.initialValue ?? 0}
                    min={data.min}
                    max={data.max}
                    step={2}
                    onInput={(val) => handleSliderChange(data.id, val)}
                  />
                </div>
              ))}
          </div>
        </div>
      </div>
    </div>
  );
}
