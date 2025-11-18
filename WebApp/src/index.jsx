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
import LandingPage from "./pages/landingPage";
import "./index.css";

/***************************************************************
 * @brief
 * 
 * @returns 
 ***************************************************************/
const Header = () => {
  return (
    <div>
      <h1>Maybe have a top bar for spacing and other global buttons?</h1>
    </div>
  );
}

/****************************************************************
 * @brief
 * 
 * @returns 
 ****************************************************************/
const Footer = () => {
  return (
    <div>
      <h1>Maybe use a footer?</h1>
    </div>
  );  
}

/***************************************************************
 * @brief
 * 
 * @returns 
 ***************************************************************/
const AppContent = () => {
  return (
    <Router>
      <Route path="/" component={LandingPage} />
      {/* TODO: maybe give these routes permissions? */}
      {/* <Route path="/network" component={NetworkPage} /> */}
      {/* <Route path="/modules" component={ModulesPage} /> */}
    </Router>
  );
}

/***************************************************************
 * @brief
 * 
 * @returns 
 ***************************************************************/
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
