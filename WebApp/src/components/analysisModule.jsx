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

// TODO: pass in a interface prop to define different knobs, sliders, etc.
export default function AnalysisModule() {
  const [activeGenre, setActiveGenre] = useState("Rock");
  const [knobValue, setKnobValue] = useState({});
  const [sliderValue, setSliderValue] = useState({});

  // FIXME: simple example callback function used for the Slider
  /*const handleInput = (id, value) => {
    console.log(String(id) + String("Value: ") + String(value));
  };*/

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

  const currentSliders = EQ_PRESETS[activeGenre];
  const currentKnobs = EQ_PRESETS[activeGenre];

  return (
    <div className="p-4">
      <h1 className="text-xl font-bold mb-4">EQ</h1>

      <div style={{ display: "flex", gap: "10px", marginBottom: "30px" }}>
        {Object.keys(EQ_PRESETS).map((genre) => (
          <button
            className={`pr-2.5 pl-2.5 pt-5 pb-5 border border-solid border-[#ccc] rounded-lg cursor-pointer
            ${
              activeGenre === genre
                ? "bg-[#fcd34d] font-bold"
                : "bg-[#e5e7eb] font-normal"
            }`}
            key={genre}
            onClick={() => setActiveGenre(genre)}
          >
            {" "}
            {genre}{" "}
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
      </div>

      <div className="flex flex-row gap-5 p-5">
        {currentSliders.map((slider) => (
          <Slider
            key={slider.id}
            title={slider.title}
            initialValue={0}
            value={sliderValue[slider.id] ?? slider.default ?? 0}
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
  );
}
