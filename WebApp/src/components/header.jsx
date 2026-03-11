/***************************************************************
 * File: header.jsx
 *
 * Date: 03/10/2026
 *
 * Description: The header component for the web app.
 *
 * Author: Ivan Wong and Bella Mann
 ***************************************************************/

import { useState, useEffect } from "preact/hooks";
import { api, HTTP_STATUS } from "../utils/utils";
import { Match } from "preact-router/match";

/**
 * @brief Displays the app header
 *
 * @returns Header component
 */

const Header = () => {
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

  const printMemory = async () => {
    const res = await api("GET", "/dev/getMemory");

    if (res?.status == HTTP_STATUS.OK) {
      console.log("Check serial monitor");
    }
  };

  return (
    <div className="p-4 flex w-full min-h-[8vh] justify-between">
      
      {/* Left side */}
      <div className="flex flex-row justify-between">
        {/* TODO: use the vibrosonics logo file in an assets folder under WebApp/assets and link img here */}
        <h2 className="font-bold text-3xl ml-2">Vibrosonics</h2>
      </div>

      {/* Right side */}
      <div>
        <div>
          <nav className="pt-2">
            <NavLink to="/">Home</NavLink>
            <NavLink to="/network">Networks</NavLink>
            <NavLink to="/modules">Modules</NavLink>
          </nav>

          <div className="pt-4">
            DEBUGGING
            <button className="ml-3 bg-cyan-400 cursor-pointer" onClick={printMemory}>
              PRINT MEMORY BTN
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};

export default Header;
