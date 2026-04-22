/***************************************************************
 * File: knob.jsx
 *
 * Date: 11/17/2025
 *
 * Description: Generic UI knob component with step control
 *
 * Author: Ivan Wong and Bella Mann
 ***************************************************************/

import { useEffect, useRef } from "preact/hooks";
import InfoButton from "./infoButton";

/**
 * @brief The Slider component is a skeleton for an audio analysis setting which uses a range based knob
 *        for reconfiguration
 *
 * @param {Object} setting - Expanded object for the knob settings
 * @param {String} setting.title - Name of the setting to be changed
 * @param {String} [setting.description] - description of setting 
 * @param {Number} setting.value - changing value
 * @param {Number} setting.min - Min value the knob can be at
 * @param {Number} setting.max - Max value the knob can be at
 * @param {Number} setting.step - Step size for each knob increment
 * @param {CallableFunction} setting.onChange - Callback that happens for each knob value change
 */
export default function Knob({
  min = 0,
  max = 10,
  step = 1,
  onChange,
  title,
  description,
  value,
}) {
  const startY = useRef(null);
  const startVal = useRef(null);

  const percentage = (value - min) / (max - min);
  const rotation = -135 + percentage * 270;

  const handleMouseDown = (e) => {
    e.preventDefault();
    startY.current = e.clientY;
    startVal.current = value;
    window.addEventListener("mousemove", handleMouseMove);
    window.addEventListener("mouseup", handleMouseUp);
  };

  const handleMouseMove = (e) => {
    if (startY.current === null) return;

    const sensitivity = 100;
    const deltaY = startY.current - e.clientY;

    const change = (deltaY / sensitivity) * (max - min);
    let rawVal = startVal.current + change;
    let newVal = Math.round(rawVal / step) * step;

    if (newVal > max) newVal = max;
    if (newVal < min) newVal = min;

    if (onChange) onChange(newVal);
  };
  const handleMouseUp = () => {
    startY.current = null;
    window.removeEventListener("mousemove", handleMouseMove);
    window.removeEventListener("mouseup", handleMouseUp);
  };

  const handleSpinboxChange = (e) => {
    const val =
      e.target instanceof HTMLInputElement ? Number(e.target.value) : value;
    const clampedVal = Math.max(min, Math.min(max, val));

    if (onChange) onChange(clampedVal);
  };

  useEffect(() => {
    return () => {
      window.removeEventListener("mousemove", handleMouseMove);
      window.removeEventListener("mouseup", handleMouseUp);
    };
  }, []);

  return (
    // Parent container
    <div className="flex flex-col items-center font-bold text-sm">

      {/* Outside slider */}
      <div className="w-[100px] h-[100px] flex items-center justify-center relative">
        <div
          onMouseDown={handleMouseDown}
          className="group w-[75px] h-[75px] bg-gray-800 border-2 border-gray-600 rounded-full relative cursor-ns-resize shadow-lg transition-colors"
        >
          {/* Knob */}
          <div className="w-[75px] h-[75px] bg-gray-800 border-2 rounded-full">
            {/* Rotator */}
            <div
              className="w-full h-full absolute top-0 left-0 rounded-full pointer-events-none"
              style={{ transform: `rotate(${rotation}deg)` }}
            >
              <div className="w-1.5 h-3 bg-white mx-auto mt-2 rounded-full shadow-[0_0_5px_white]" />
            </div>
          </div>
        </div>
      </div>
      <div className="flex flex-row">
        <div className="mt-1 justify-center">{title}</div>
        {description && (
          <div className="asbolute top-2 left-2">
            <InfoButton 
                infoText={description} 
                onClick={() => {}} // Pass an empty function so it doesn't crash if clicked
                showToolTip={true}
            />
          </div>
        )}
      </div>

      {/* Spinbox for the knob */}
      <div className="flex flex-col items-center gap-1 font-semibold">
        <input
          type="number"
          className="w-32 pt-1 pb-1 pl-2 pr-2 text-center bg-white border border-gray-400 rounded text-sm"
          value={value.toFixed(2)}
          onChange={(e) => handleSpinboxChange(e)}
          min={min}
          max={max}
          step={step}
        />
      </div>
    </div>
  );
}
