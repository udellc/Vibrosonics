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
import "../index.css";

export function Knob({ value, min, max, step, onChanged }) {
  const [currentValue, setCurrentValue] = useState(value);
  const [isDragging, setIsDragging] = useState(false);

  return (
    // Parent container
    <div className="w-full h-full flex items-center justify-center font-bold text-sm">

      {/* Outside slider */}
      <div className="w-[100px] h-[100px] flex items-center justify-center border-2 relative">

        {/* Knob */}
        <div className="w-[75px] h-[75px] bg-gray-800 border-2 rounded-full">
          
          {/* Rotator */}
          <div>
            
          </div>
          {/* Text value */}
          <div>
            
          </div>
        </div>
      </div>
    </div>
  );
}
