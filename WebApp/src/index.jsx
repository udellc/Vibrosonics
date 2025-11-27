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

/**
 * @brief
 *
 * @returns
 */
const AppContent = () => {
  return (
    <Router>
      <Route path="/" component={LandingPage} />
      {/* TODO: maybe give these routes permissions? */}
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
    <>
      <Header />
      <AppContent />
      <Footer />
    </>
  );
}

render(<App />, document.getElementById("app"));
