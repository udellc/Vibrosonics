/***************************************************************
 * File: knob.jsx
 *
 * Date: 11/17/2025
 *
 * Description: Generic UI knob component with step control
 *
 * Author: Ivan Wong
 ***************************************************************/

import { useState } from "preact/hooks";

/**
 * @brief The knob component is a skeleton for audio analysis setting which uses knob like
 * UI component for reconfiguration
 * 
 * @param { Object } _ - Expanded object for the knob settings
 * @param { String } _.title - Name of setting to be changed
 * @param { Number } _.initialValue - Initial value of the knob
 * @param { Number } _.min - Min value for the knob range
 * @param { Number } _.max - Max value for the knob range
 * @param { Number } _.step - Step size for each knob increment
 * @param { CallableFunction } _.onChange - Callback that happens for each knob value change
 */
export default function Knob({ title, initialValue, min, max, step, onChange }) {
  const [value, setValue] = useState(initialValue);
  const [isDragging, setIsDragging] = useState(false);

  return (
    // Parent container
    <div className="flex items-center font-bold text-sm">
      
      {/* Outside slider */}
      <div className="w-[100px] h-[100px] flex items-center justify-center border-2 relative">

        {/* Knob */}
        <div className="w-[75px] h-[75px] bg-gray-800 border-2 rounded-full">
          
          {/* Rotator */}
          <div>
            
          </div>

          {/* Text value */}
          <div>
            <span>{title}: {value}</span>
          </div>
        </div>
      </div>
    </div>
  );
}
