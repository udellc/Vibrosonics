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
import RadioPage from "./pages/radioPage";
import Header from "./components/header";
import Footer from "./components/footer";
import {
  AnalysisSettingsProvider,
  SystemContext,
  SystemContextProvider,
} from "./utils/configurations";
import { useContext, useEffect, useState } from "preact/hooks";
import { api, PAGE } from "./utils/utils";

/**
 * @brief Defines the different pages for the main App
 *
 * @returns Routes to each page component
 */
const AppContent = () => {
  const { setPageInfo } = useContext(SystemContext);

  // Used to update header when page changes
  const onPageChange = (e) => {
    if (e.url === "/") setPageInfo(PAGE.LANDING);
    else if (e.url === "/network") setPageInfo(PAGE.NETWORK);
    else if (e.url === "/modules") setPageInfo(PAGE.MODULES);
    else if (e.url === "/radio") setPageInfo(PAGE.RADIO);
  };
  return (
    <Router onChange={onPageChange}>
      <Route path="/" component={LandingPage} />
      <Route path="/network" component={NetworkPage} />
      <Route path="/modules" component={ModulesPage} />
      <Route path="/radio" component={RadioPage} />
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
    <div className="min-w-lvw min-h-lvh flex flex-col">
      
      {/* Wrap the app content with the contexts */}
      <SystemContextProvider>
        <Header />
        <AnalysisSettingsProvider>
          <AppContent />
        </AnalysisSettingsProvider>
        <Footer />
      </SystemContextProvider>
    </div>
  );
}

render(<App />, document.getElementById("app"));
