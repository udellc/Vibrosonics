/***************************************************************
 * File: index.jsx
 * Date: 10/30/2025
 * Description: The main entry point for the VibroSonics web
 * 				application built with Preact and Vite.
 * Author: Ivan Wong
 ***************************************************************/

import { render } from "preact";
import Router, { Route } from "preact-router";
import LandingPage from "./pages/landingPage";
import "./index.css";

export function App() {
  return (
    <Router>
      <Route path="/" component={LandingPage} />
    </Router>
  );
}

render(<App />, document.getElementById("app"));
