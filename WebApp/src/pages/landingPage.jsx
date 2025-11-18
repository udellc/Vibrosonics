/***************************************************************
 * File: landingPage.jsx
 * Date: 11/06/2025
 * Description: The landing page for web app
 * Author: Ivan Wong
 ***************************************************************/

import "../index.css";
import { Knob } from "../atomics/knob";

const LandingPage = () => {
  // FIXME: remove
  const exampleCallback = (value) => {
    console.log("Value: " + value)
  }

  return (
    <div>
      <h1 className="font-bold mt-10">
        Landing page title
      </h1>
      <div>
        <p>Maybe a page to connect to a wifi network</p>
        
        {/* FIXME: remove later */}
        <div className="w-1/2">
          <Knob value={0} min={0} max={0} step={1} onChanged={exampleCallback}/>
        </div>
      </div>
    </div>
  );
};

export default LandingPage;
