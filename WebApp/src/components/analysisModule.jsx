/***************************************************************
 * File: analysisModule.jsx
 *
 * Date: 11/19/2025
 *
 * Description: UI component for an audio analysis module
 *
 * Author: Ivan Wong
 ***************************************************************/

import Slider from "../atomics/slider";

export default function AnalysisModule() {
  const handleInput = (value) => {
    console.log("Value: " + value);
  };

  return (
    <div>
      {/* TODO: remove this h1 title */}
      <h1>Analysis module</h1>
      <Slider
        initialValue={0}
        min={0}
        max={100}
        step={1}
        onInput={handleInput}
      />
    </div>
  );
}
