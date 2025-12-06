/***************************************************************
 * File: networkPage.jsx
 *
 * Date: 11/22/2025
 *
 * Description: The network page for the web app. Handles the UI
 * connections for the networking API calls
 *
 * Author: Ivan Wong
 ***************************************************************/

import { useEffect, useState } from "preact/hooks";
// import { route } from "preact-router";
import { api } from "../utils/utils";
import NetworkCard from "../components/networkCard";
import TextEntry from "../components/textEntry";

/**
 * @brief
 */
const NetworkPage = () => {
  const [availableNetworks, setAvailableNetworks] = useState([]);
  const [selectedNetwork, setSelectedNetwork] = useState("");
  const [password, setPassword] = useState("");
  const [showTextForm, setTextForm] = useState(false);

  /**
   * @brief Makes a request to the ESP32 to scan and return available networks
   */
  const getNetworks = async () => {
    try {
      const res = await api("GET", "/network/scanNetworks");

      if (res) {
        setAvailableNetworks(res.ssid);
      }
    } catch (err) {
      console.error("Failed to scan networks", err);
    }
  };
  /**
   * @brief Gets the reponse to a network conenction request
   */
  const getNetworkResponse = async () => {
    const payload = {
      selectedNetwork,
      password,
    };
    const res = await api("POST", "/network/connect", payload);

    return res;
  }
  /**
   * @brief Shows the password form and sets the selected network
   *
   * @param { String } SSID - User selected network SSID
   */
  const handleConnectClicked = (SSID) => {
    setTextForm(true);
    setSelectedNetwork(SSID);
  };
  /**
   * @brief Sends a connection request to the web server for the selected network SSID and password.
   * Routes to the modules page on successful connection
   */
  const handleNetworkRequest = () => {
    const res = getNetworkResponse();

    console.log(res);
  };
  // Scan for networks on mount
  useEffect(() => {
    getNetworks();
  }, []);

  return (
    <div className="mt-20 ml-10 mr-10">
      {/* Centered vertical layout */}
      <div className="flex flex-col items-center">
        <h1 className="font-bold mt-10 text-4xl">Available Networks</h1>

        {/* Scan network button */}
        <button
          className="border-3 border-amber-500 p-1 cursor-pointer mt-3 text-xl font-semibold rounded-sm"
          onClick={getNetworks}
        >
          Scan Networks
        </button>

        {/* Network list */}
        {availableNetworks.map((network) => {
          return (
            <NetworkCard
              key={network}
              SSID={network}
              onConnect={() => {
                handleConnectClicked(network);
              }}
            />
          );
        })}
        {showTextForm === true ? (
          // Form for entering password
          <>
            <p className="mt-4">Selected Network: {selectedNetwork}</p>
            <TextEntry
              label="Password"
              entryType="password"
              presetText="Enter Password"
              onChange={setPassword}
            />
            <button
              className="border-3 border-amber-500 p-1 cursor-pointer mt-3 text-lg font-semibold rounded-sm"
              onClick={handleNetworkRequest}
            >
              Submit
            </button>
          </>
        ) : (
          // Show nothing
          <></>
        )}
      </div>
    </div>
  );
};

export default NetworkPage;
