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
import { route } from "preact-router";
import { api, HTTP_STATUS } from "../utils/utils";
import NetworkCard from "../components/networkCard";
import TextEntry from "../components/textEntry";

/**
 * @brief
 */
const NetworkPage = () => {
  const [availableNetworks, setAvailableNetworks] = useState([]);
  const [isLoading, setIsLoading] = useState(false);
  const [selectedNetwork, setSelectedNetwork] = useState("");
  const [password, setPassword] = useState("");
  const [showTextForm, setTextForm] = useState(false);

  /**
   * @brief Makes a request to the ESP32 to scan and return available networks
   */
  const getNetworks = async () => {
    setIsLoading(true);

    try {
      const res = await api("GET", "/network/scanNetworks");

      if (res.status == HTTP_STATUS.OK) {
        setAvailableNetworks(res.data.ssid);
      }
    } catch (err) {
      console.error("Failed to scan networks", err);
    } finally {
      setIsLoading(false);
    }
  };
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
  const handleNetworkRequest = async () => {
    const payload = {
      selectedNetwork,
      password,
    };
    const res = await api("POST", "/network/connect", payload);

    if (res.status == HTTP_STATUS.ACCEPTED) {
      // TODO: update some sort of context that the header/footer use to show disconnect option
      route("/modules", true);
    }
  };

  // Scan for networks on mount
  useEffect(() => {
    getNetworks();
  }, []);

  return (
    <div className="mt-20 min-h-[60vh] ml-10 mr-10">
      {isLoading ? (
        
        <div className="flex flex-col items-center gap-3">
          <div className="w-10 h-10 border-4 border-gray-200 border-t-amber-500 rounded-full animate-spin" />
          <p className="text-gray-500 font-medium animate-pulse">
            Scanning for networks...
          </p>
        </div>

      ) : (

        <div className="flex flex-col items-center">
          {/* Centered vertical layout */}
          <h1 className="font-bold mt-10 text-4xl">Available Networks</h1>

          {/* Scan network button */}
          <div className="pt-4">
            <button
              className="p-3 bg-amber-200 border border-amber-500 rounded-lg cursor-pointer hover:bg-[#fbbf24]"
              onClick={getNetworks}
            >
              Scan Networks
            </button>
          </div>

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
                className="p-2 bg-amber-200 border border-amber-500 cursor-pointer mt-3 text-lg font-semibold rounded-sm hover:bg-[#fbbf24]"
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

      )}
    </div>
  );
};

export default NetworkPage;
