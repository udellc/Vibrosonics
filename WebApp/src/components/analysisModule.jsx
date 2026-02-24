/***************************************************************
 * File: analysisModule.jsx
 *
 * Date: 11/19/2025
 *
 * Description: UI component for an audio analysis module
 *
 * Author: Ivan Wong and Bella Mann
 ***************************************************************/

import { useEffect, useState } from "preact/hooks";
import Knob from "../atomics/knob";
import ModuleDisplay from "../data/moduleDisplay.json";
import { FREQUENCY_MAPPING, MODULE_TYPE } from "../utils/utils";

// TODO: pass in a interface prop to define different knobs, sliders, etc.
export default function AnalysisModule({ index, module, setModules }) {
  const moduleSettingsDisplay = ModuleDisplay.module.settings;
  const knobSettings = ["freqLow", "freqHigh", "minAmpNorm"];
  const [isValid, setIsValid] = useState(true);

  const updateValue = (id, val) => {
    setModules((prev) => {
      const updated = [...prev];
      updated[index] = {
        ...updated[index],
        [id]: val,
      };
      return updated;
    });
  };

  const handleKnobChange = (id, val) => {
    updateValue(id, val);
  };
  const handleValueChange = (id, value) => {
    // TODO: fix later
    if (id === "maxPeaks") {
      updateValue(id, value);
      return;
    }
    const numValue = Number(value);
    const maxValue = moduleSettingsDisplay.knobs[id].max;
    const minValue = moduleSettingsDisplay.knobs[id].min;
    const clampedVal = Math.max(minValue, Math.min(maxValue, numValue));

    updateValue(id, clampedVal);
  };
  // Ensure frequency ranges are valid for low/high
  useEffect(() => {
    const freqLow = module.freqLow;
    const freqHigh = module.freqHigh;
    const isValidFreqRanges = (freqLow < freqHigh) && (freqHigh > freqLow);

    // Only update is value is flipped
    if (isValidFreqRanges !== isValid) {
      setIsValid(!isValid);
    }
  }, [module.freqLow, module.freqHigh]);
  return (
    <div className="pt-8 p-4 bg-gray-200 rounded-xl shadow-inner flex flex-col items-center">
      <h3 className="font-bold text-lg">
        Module: {MODULE_TYPE[module.moduleType]}
      </h3>
      <h3 className="font-bold text-lg">TODO: Index {index}</h3>

      {/* TODO: this is some logic saying ranges are not valid, add some sort of handling here */}
      <div>
        {isValid? (
          <div>TODO: Valid ranges</div>
        ) : (
          <div className="font-bold text-red-500">TODO: Not valid ranges</div>
        )}
      </div>

      {/* Row layout */}
      <div className="flex flex-row">
        {/* Knobs for base module */}
        <div className="flex flex-col gap-1">
          {knobSettings.map((key) => {
            return (
              <div>
                <Knob
                  min={moduleSettingsDisplay.knobs[key].min}
                  max={moduleSettingsDisplay.knobs[key].max}
                  title={moduleSettingsDisplay.knobs[key].title}
                  step={moduleSettingsDisplay.knobs[key].step}
                  onChange={(value) => handleKnobChange(key, value)}
                  value={Number(parseFloat(module[key] ?? 0).toFixed(2))}
                />
                <div className="flex flex-col items-center gap-1">
                  <input
                    type="number"
                    className="w-fit pt-1 pb-1 pl-2 pr-2 text-center bg-white border border-gray-400 rounded text-sm"
                    value={Number(parseFloat(module[key] ?? 0).toFixed(2))}
                    onChange={(e) =>
                      handleValueChange(
                        key,
                        e.target instanceof HTMLInputElement
                          ? Number(e.target.value)
                          : 0,
                      )
                    }
                    min={moduleSettingsDisplay.knobs[key].min}
                    max={moduleSettingsDisplay.knobs[key].max}
                  />
                </div>
              </div>
            );
          })}
        </div>
        {/* Module specific params */}

        {/* Frequency mapping */}
        <div className="p-6 flex flex-col gap-2 items-center">
          <div className="flex-col items-center bg-amber-100 max-w-fit">
            <h4 className="font-bold">Frequency<br/>Mapping</h4>
            <select name="freqMaps" value={module["frequencyMapping"] ?? 0}>
              <option value={0}>None</option>
              <option value={1}>Octave</option>
              <option value={2}>Midi</option>
            </select>
          </div>
          <div>
            <h4 className="font-bold">Max Peaks</h4>
            <input
              type="number"
              value={module["maxPeaks"] ?? 1}
              onChange={(e) =>
                handleValueChange(
                  "maxPeaks",
                  e.target instanceof HTMLInputElement
                    ? Number(e.target.value)
                    : 1,
                )
              }
              min={1}
              max={32}
            />
          </div>
        </div>
      </div>
    </div>
  );
}
