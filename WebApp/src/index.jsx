/***************************************************************
 * File: index.jsx
 * Date: 10/30/2025
 * Description: The main entry point for the VibroSonics web
 * 				application built with Preact and Vite.
 * Author: Ivan Wong
 ***************************************************************/

import { render } from "preact";
import "./index.css";

export function App() {
  return (
    <div>
      <h1>Hello VibroSonics</h1>
      <p>Test</p>
    </div>
  );
}

render(<App />, document.getElementById("app"));
