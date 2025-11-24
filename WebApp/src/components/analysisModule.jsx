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
import { EQ_PRESETS } from "../atomics/eqSettings";
import { useState } from 'react';

// TODO: pass in a interface prop to define different knobs, sliders, etc.
export default function AnalysisModule() {
  
  const [activeGenre, setActiveGenre] = useState('Rock');
  const [values, setValues] = useState({});
  
  // FIXME: simple example callback function used for the Slider
  const handleInput = (id, value) => {
    console.log(String(id) + String("Value: ") + String(value));
    setValues(prev => ({...prev, [id]: value }));
  };

  const currentSliders = EQ_PRESETS[activeGenre];
  const currentKnobs = EQ_PRESETS[activeGenre];

  return (
    <div className="p-4">
      <h1 className="text-xl font-bold mb-4">EQ</h1>

      <div style={{display: 'flex', gap: '10px', marginBottom: '30px'}}>
        {Object.keys(EQ_PRESETS).map((genre) => (
          <button
            key={genre}
            onClick={() => setActiveGenre(genre)}
            style={{
              padding: '10px 20px',
              backgroundColor: activeGenre === genre ? '#fcd34d' : '#e5e7eb',
              fontWeight: activeGenre == genre ? 'bold' : 'normal',
              border: '1px solid #ccc',
              borderRadius: '8px',
              cursor: 'pointer'
            }}
          > {genre} </button>
        ))}
      </div>

      <div className="flex flex-wrap gap-8 p-8 bg-gray-200 rounded-xl shadow-inner max-w-3xl">
        {currentKnobs.map((knobData) => (
          <div key={knobData.id} className="flex flex-col items-center gap-2">
            <Knob
              value={values[knobData.id] ?? knobData.default}
              min={0}
              max={100}
              step={1}
              onChange={(val) => handleInput(knobData.id, val)}
          />
          <span className = "font-bold text-gray-700">{knobData.label}</span>
        </div>
        ))}
      </div>

      <div style={{
        display: 'flex',
        flexDirection: 'row',
        gap: '20px',
        padding: '20px'
      }}>
        {currentSliders.map((slider) => (
        <Slider
          key={slider.id}
          title={slider.title}
          initialValue={0}
          min={slider.min}
          max={slider.max}
          step={2}
          onInput={(val) => handleInput(slider.id, val)}
        />
        ))}
      </div>
    </div>
  );
}
