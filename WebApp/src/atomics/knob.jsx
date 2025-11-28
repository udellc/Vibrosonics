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

/**
 * @brief The Slider component is a skeleton for an audio analysis setting which uses a range based knob
 *        foe reconfiguration
 *
 * @param {Object} setting - Expanded object for the knob settings
 * @param {String} setting.title - Name of the setting to be changed
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

  useEffect(() => {
    return () => {
      window.removeEventListener("mousemove", handleMouseMove);
      window.removeEventListener("mouseup", handleMouseUp);
    };
  }, []);

  return (
    // Parent container
    <div className="flex items-center font-bold text-sm">
      {/* Outside slider */}
      <div className="w-[100px] h-[100px] flex items-center justify-center border-2 relative">
        <div
          onMouseDown={handleMouseDown}
          className="w-[75px] h-[75px] bg-gray-800 border-2 border-gray-600 rounded-full relative cursor-ns-resize shadow-lg active:border-yellow-400 transition-colors"
        >
          {/* Knob */}
          <div className="w-[75px] h-[75px] bg-gray-800 border-2 rounded-full">
            {/* Rotator */}
            <div
              className="w-full h-full absolute top-0 left-0 rounded-full pointer-events-none"
              // FIXME: convert to tailwindcss, `rotate-${rotation}` does not work as expected
              style={{ transform: `rotate(${rotation}deg)` }}
            >
              <div className="w-1.5 h-3 bg-white mx-auto mt-2 rounded-full shadow-[0_0_5px_white]" />
            </div>

            {/* Text value */}
            <div className="absolute inset-0 flex items-center justify-center text-white pointer-events-none">
              {title}
            </div>
          </div>
        </div>
      </div>
      Value: {Math.round(value)} {/* change this to be below*/}
    </div>
  );
}
