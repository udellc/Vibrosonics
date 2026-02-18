/***************************************************************
 * File: index.jsx
 *
 * Date: 10/30/2025
 *
 * Description: The main entry point for the VibroSonics web
 * application built with Preact and Vite.
 *
 * Author: Ivan Wong
 ***************************************************************/

import { render } from "preact";
import Router, { Route } from "preact-router";
import "./index.css";
import LandingPage from "./pages/landingPage";
import NetworkPage from "./pages/networkPage";
import ModulesPage from "./pages/modulesPage";
import Header from "./components/header";
import Footer from "./components/footer";
import { AudioSettingsProvider } from "./utils/configurations";

/**
 * @brief Defines the different pages for the main App
 *
 * @returns Routes to each page component
 */
const AppContent = () => {
  return (
    <Router>
      <Route path="/" component={LandingPage} />
      <Route path="/network" component={NetworkPage} />
      <Route path="/modules" component={ModulesPage} />
    </Router>
  );
};

/**
 * @brief
 *
 * @returns
 */
export function App() {

  return (
    // FIXME: align the footer to the bottom with the header and content filling the rest of space
    <div className="min-w-lvw min-h-lvh flex flex-col">
      <Header />
      {/* Wrap the app content with the audio context */}
      <AudioSettingsProvider>
        <AppContent />
      </AudioSettingsProvider>
      <Footer/>
    </div>
  );
}

render(<App />, document.getElementById("app"));
