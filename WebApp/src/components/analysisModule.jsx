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

// TODO: pass in a interface prop to define different knobs, sliders, etc.
export default function AnalysisModule() {
  
  // FIXME: simple example callback function used for the Slider
  const handleInput = (value) => {
    console.log("Value: " + value);
  };

  return (
    <div>
      <Slider
        title={"example of the slider"}
        initialValue={0}
        min={0}
        max={100}
        step={2}
        onInput={handleInput}
      />
    </div>
  );
}
