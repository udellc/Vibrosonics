/***************************************************************
 * File: header.jsx
 *
 * Date: 03/10/2026
 *
 * Description: The header component for the web app.
 *
 * Author: Ivan Wong and Bella Mann
 ***************************************************************/

import { Match } from "preact-router/match";
import logo from "../images/cymaspaceLogo.jpg"

/**
 * @brief Displays the app header
 *
 * @returns Header component
 */
const Header = ({ setCurrentPage }) => {    //eslint-disable-line no-unused-vars
  const NavLink = ({ to, children }) => {

    const baseClasses = "p-4 px-4 py-2 rounded-md font-medium transition-colors duration-200";
    const activeClasses = "bg-gray-400 text-white shadow-sm";
    const inactiveClasses = "text-gray-600 hover:bg-gray-100 hover:text-gray-900";

    return(
      <Match path={to}>
        {({ url }) => {
          const isActive = url === to;

          return(
            <a
              href={to}
              className={`${baseClasses} ${isActive? activeClasses : inactiveClasses}`}
            >
              {children}
            </a>
          );
        }}
      </Match>
    );
  };

  return (
    <div className="p-4 flex w-full min-h-[8vh] justify-between items-center">
      
      {/* Left side */}
      <div className="flex flex-row justify-between items-center">
        <img src={logo} alt="Image of the Cymaspace logo." className="w-10 h-10" /> {/** TODO: add actual alt text */}
        <h2 className="font-bold text-2xl ml-2">Vibrosonics</h2>
      </div>

      {/* Right side */}
      <div>
        <div>
          <nav id="main-nav">
            <NavLink to="/">Home</NavLink>
            <NavLink to="/network">Networks</NavLink>
            <NavLink to="/modules">Modules</NavLink>
            <NavLink to="/radio">FM Radio</NavLink>
            {/** CHANGE LINK BELOW TO WEBSITE */}
            <NavLink to="https://www.cymaspace.org/">CymaSpace</NavLink>
            <NavLink to="https://github.com/udellc/Vibrosonics">GitHub Repo</NavLink>
          </nav>
        </div>
      </div>
    </div>
  );
};

export default Header;
