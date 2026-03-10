/***************************************************************
 * File: header.jsx
 *
 * Date: 11/18/2025
 *
 * Description: The header component for the web app.
 *
 * Author: Ivan Wong and Bella Mann
 ***************************************************************/

import { useState, useEffect } from "preact/hooks";
import { api, HTTP_STATUS } from "../utils/utils";

/**
 * @brief Displays the app header
 *
 * @returns Header component
 */

const Header = () => {
  const [route, setRoute] = useState(window.location.pathname);

  useEffect(() => {
    const handleLocationChange = () => {
      setRoute(window.location.pathname);
    };

    window.addEventListener('popstate', handleLocationChange);
    return () => window.removeEventListener('popstate', handleLocationChange);
  }, []);

  const navigate = (path) => {
    window.history.pushState({}, '', path);
    setRoute(path);
  };

  const NavLink = ({ to, children}) => (
    <a
      href={to}
      onClick={(e) => {
        e.preventDefault();
        navigate(to);
      }}
      style={{ marginRight: '10px' }}
    >
      {children}
    </a>
  )

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
          <nav>
            <NavLink to="/">Home</NavLink>
            <NavLink to="/network">Networks</NavLink>
            <NavLink to="/modules">Modules</NavLink>
          </nav>

          DEBUGGING
          <button className="ml-3 bg-cyan-400 cursor-pointer" onClick={printMemory}>
            PRINT MEMORY BTN
          </button>
        </div>
      </div>
    </div>
  );
};

export default Header;
