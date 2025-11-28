/***************************************************************
 * File: slider.jsx
 *
 * Date: 11/17/2025
 *
 * Description: Generic UI slider component with step control
 *
 * Author: Ivan Wong
 ***************************************************************/

import { useState } from "preact/hooks";

/**
 * @brief The Slider component is a skeleton for an audio analysis setting which uses a range based slider
 *        for reconfiguration
 * 
 * @param {Object} setting - Expanded object for the slider settings
 * @param {String} setting.title - Name of the setting to be changed
 * @param {Number} setting.initialValue - Initial/Default value
 * @param {Number} setting.min - Min value the slider can be at
 * @param {Number} setting.max - Max value the slider can be at
 * @param {Number} setting.step - Step size for each slider increment
 * @param {CallableFunction} setting.onInput - Callback that happens for each slider value change
 */
export default function Slider({ title, initialValue, min, max, step, onInput }) {
  const [value, setValue] = useState(initialValue);

  // Updates the value UI for each slider movement
  const handleInput = (e) => {
    const updatedVal = Number(e.target.value);
    setValue(updatedVal);
  };

  // Calls the custom callback function when the value settles to a number
  const handleInputChanged = (e) => {
    const updatedVal = Number(e.target.value);
    onInput?.(updatedVal);
  };

  return (
    <div className="flex flex-col w-max h-max items-center m-1 p-2">
      <input className="[writing-mode:vertical-lr] [direction:rtl]"
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onInput={handleInput}
        onChange={handleInputChanged}
      />
      <span>{title}: {value}</span>
    </div>
  );
}
