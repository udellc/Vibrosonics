/***************************************************************
 * File: index.jsx
 * Date: 10/30/2025
 * Description: The main entry point for the VibroSonics web
 * 				application built with Preact and Vite.
 * Author: Ivan Wong
 ***************************************************************/

import { render } from "preact";
import { LandingPage } from "./pages/landingPage";
import "./index.css";

export function App() {
  return (
    <div>
      <LandingPage />
    </div>
  );
}

render(<App />, document.getElementById("app"));
