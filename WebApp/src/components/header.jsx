/***************************************************************
 * File: header.jsx
 *
 * Date: 11/18/2025
 *
 * Description: The header component for the web app.
 *
 * Author: Ivan Wong
 ***************************************************************/

import { useState } from "preact/hooks";
import { api, HTTP_STATUS } from "../utils/utils";

/**
 * @brief Displays the app header
 *
 * @returns Header component
 */
const Header = () => {
  const [data, setData] = useState("");
  const printMemory = async () => {
    const res = await api("GET", "/dev/getMemory");

    if (res?.status == HTTP_STATUS.OK) {
      setData(res.data);
    }
  }

  return (
    <div className="p-4 flex w-full h-[10vh] border-b-2 border-black justify-between">

      {/* Left side */}
      <div className="flex flex-row justify-between">

        {/* Logo can go here */}
        {/* TODO: use the vibrosonics logo file in an assets folder under WebApp/assets and link img here */}
        <h2 className="font-bold text-3xl ml-2">
          Vibrosonics
        </h2>
      </div>

      {/* Right side */}
      <div>
        <div>DEBUGGIN
          <button
            className="bg-cyan-400 cursor-pointer"
            onClick={printMemory}
          >PRINT MEMORY</button>
          {data}
        </div>
        <div>Network stuff</div>
      </div>
    </div>
  );
};

export default Header;
