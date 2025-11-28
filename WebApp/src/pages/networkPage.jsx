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
import { api } from "../utils/utils";
import NetworkCard from "../components/networkCard";
import TextEntry from "../components/textEntry";

/**
 * @brief
 */
const NetworkPage = () => {
  const [availableNetworks, setAvailableNetworks] = useState([]);
  const [selectedNetwork, setSelectedNetwork] = useState("");
  const [showTextForm, setTextForm] = useState(false);

  /**
   * @brief Makes a request to the ESP32 to scan and return available networks
   */
  const fetchNetworks = async () => {
    try {
      const res = await api("GET", "/network/scanNetworks");
      if (res) {
        setAvailableNetworks(res);
      }
      console.log(String(res));
    } catch (err) {
      console.error("Failed to scan networks", err);
    }
  };
  /**
   * @brief Shows the password text entry and sets the selected network
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
   *
   * @param { String } Password - Password entered by the user
   */
  const handleNetworkRequest = (Password) => {
    const payload = {
      "wifi-ssid": selectedNetwork,
      password: Password,
    };
    const res = api("POST", "/network/networkRequest", payload);

    // Ok, route to modules page
    if (res["status"] == 200) {
      route("/modules", true);
    } else {
      // TODO: error handle
      console.log("Invalid credentials");
    }
  };
  // Scan for networks on mount
  useEffect(() => {
    fetchNetworks();
  }, []);

  return (
    <div className="mt-20 ml-10 mr-10">
      {/* Centered vertical layout */}
      <div className="flex flex-col items-center">
        <h1 className="font-bold mt-10 text-4xl">Available Networks</h1>
        <button
          className="border-3 border-amber-500 p-1 cursor-pointer mt-3 text-xl font-semibold rounded-sm"
          onClick={fetchNetworks}
        >
          Scan Networks
        </button>
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
          // TODO: add some sort of functionality to pass in SSID and password, assuming the component
          // is form HTML type.
          <TextEntry
            presetText="FIXME"
            onEntered={() => {
              handleNetworkRequest(null);
            }}
          />
        ) : (
          // Show nothing
          <></>
        )}
      </div>
    </div>
  );
};

export default NetworkPage;
