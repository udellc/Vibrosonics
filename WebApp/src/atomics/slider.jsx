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

export default function Slider({ initialValue, min, max, step }) {
  const [value, setValue] = useState(initialValue);

  const handleInput = (e) => {
    const newValue = Number(e.target.value);
    setValue(newValue);    
  };

  return (
    <div className="flex flex-col items-center">
      <input className="w-[300px]"
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={handleInput}
      />
      <h1>Value: {value}</h1>
    </div>
  );
}
