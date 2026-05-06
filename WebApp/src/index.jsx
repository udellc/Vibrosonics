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
import OnboardingTour from "./components/OnboardingTour";
import {
  AnalysisSettingsProvider,
  SystemContext,
  SystemContextProvider,
} from "./utils/configurations";
import { useContext, useState } from "preact/hooks";
import { PAGE } from "./utils/utils";

/**
 * @brief Defines the different pages for the main App
 *
 * @returns Routes to each page component
 * @param {Object} _
 * @param {Function} _.setCurrentPage Setter for current page status
 * @param {string} _.currentPage Current page string
 * 
 */
const AppContent = ({ setCurrentPage, currentPage}) => {    //eslint-disable-line no-unused-vars
  const { setPageInfo } = useContext(SystemContext);

  // Used to update header when page changes
  const onPageChange = (e) => {
    if (e.url === "/") setPageInfo(PAGE.LANDING);
    else if (e.url === "/network") setPageInfo(PAGE.NETWORK);
    else if (e.url === "/modules") setPageInfo(PAGE.MODULES);
    else if (e.url === "/radio") setPageInfo(PAGE.RADIO);

    if(e.url === "/") 
      setCurrentPage('home');
    else 
      setCurrentPage(e.url.replace('/', ''));
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
 * 
 */
export function App() {
  const [ currentPage, setCurrentPage ] = useState('home');

  return (
    <div className="min-w-lvw min-h-lvh flex flex-col font-['Inter']">
      
      {/* Wrap the app content with the contexts */}
      <SystemContextProvider>
        <Header setCurrentPage={setCurrentPage}/>
        <AnalysisSettingsProvider>
          <AppContent currentPage={currentPage} setCurrentPage={setCurrentPage}/>
        </AnalysisSettingsProvider>
        {currentPage !== 'home' && <Footer />}
        <OnboardingTour />
      </SystemContextProvider>
    </div>
  );
}

render(<App />, document.getElementById("app"));
