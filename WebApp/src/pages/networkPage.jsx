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
import NetworkCard from "../components/networkComponents/networkCard";
import TextEntry from "../atomics/textEntry";
import wifiIcon from "../../assets/wifi.png";
import LoadingSpinner from "../atomics/loadingSpinner";
import DeviceSettings from "../components/networkComponents/deviceSettings";

const buttonStyle =
  "p-2 bg-amber-200 border border-amber-500 rounded-lg cursor-pointer hover:bg-[#fbbf24]";

/**
 * @brief Defines the network page components, allowing a user to view and connect to different networks or adjust
 * network settings.
 */
const NetworkPage = () => {
  const [availableNetworks, setAvailableNetworks] = useState([]);
  const [selectedNetwork, setSelectedNetwork] = useState("");
  const [password, setPassword] = useState("");

  // For UI
  const [isLoading, setIsLoading] = useState(false);
  const [showEnterWifiPassword, setPasswordForm] = useState(false);
  const [isConnecting, setIsConnecting] = useState(false);
  const [connectionError, setConnectionError] = useState("");

  // On-device network info
  const [externalSsid, setExternalSsid] = useState("");
  const [apSsid, setApSsid] = useState("");
  const [signal, setSignal] = useState("");
  const [apPassword, setApPassword] = useState("");

  /**
   * @brief Makes a request to the ESP32 to scan and return available networks
   */
  const getNetworks = async () => {
    setPasswordForm(false);
    setIsLoading(true);

    try {
      const res = await api("GET", "/network/scanNetworks");

      if (res?.status == HTTP_STATUS.OK) {
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
    setPasswordForm(true);
    setConnectionError("");
    setSelectedNetwork(SSID);
  };
  /**
   * @brief Sends a connection request to the web server for the selected network SSID and password.
   * Routes to the modules page on successful connection
   */
  const handleNetworkRequest = async () => {
    setConnectionError("");
    const payload = {
      selectedNetwork,
      password,
    };
    try {
      setIsConnecting(true);
      const res = await api("POST", "/network/connect", payload);

      if (res?.status == HTTP_STATUS.ACCEPTED) {
        route("/modules", true);
      } else {
        setConnectionError("Unable to Connect");
      }
    } catch (error) {
      console.error("Failed to connect to network", error);
    } finally {
      setIsConnecting(false);
    }
  };

  /**
   * @brief Gets the network info from the ESP32
   */
  const getNetworkInfo = async () => {
    /**
     * @brief Mapping function helper describing the signal strength
     *
     * @param {Number} rssi - WiFi signal strength measured in dBm
     */
    const getSignalStrength = (rssi) => {
      if (rssi >= -50) return "Excellent";
      if (rssi >= -65) return "Good";
      if (rssi >= -75) return "Fair";
      if (rssi >= -90) return "Weak";
      return "Unusable";
    };
    try {
      const res = await api("GET", "/network/getInfo");

      if (res?.status == HTTP_STATUS.OK) {
        const info = res?.data["info"];

        setExternalSsid(info.extSsid || "N/A");
        setSignal(getSignalStrength(info.rssi));
        setApSsid(info.apSsid);
        setApPassword(info.apPassword);
      }
    } catch (error) {
      console.error("Failed to get network info", error);
    }
  };

  // Scan for networks and get current network name on mount
  useEffect(() => {
    getNetworkInfo();
  }, []);

  return (
    <>
      {isConnecting ? (
        // Add overlay, so user cannot do anything that messes with the network connection
        <>
          <div className="fixed inset-0 bg-black opacity-40 z-10" />
          <div
            className={"flex fixed z-20 inset-0 items-center justify-center"}
          >
            <div
              className={
                "w-xs h-30 bg-gray-300 rounded-2xl border-2 border-black p-5"
              }
            >
              <LoadingSpinner
                size={10}
                label={`Connecting To: ${selectedNetwork}`}
              />
            </div>
          </div>
        </>
      ) : (
        <></>
      )}
      <div
        className="mt-16 min-h-[60vh] mx-8"
        // Disables tabbing into the network components
        inert={isConnecting}
        aria-hidden={isConnecting}
      >
        <h1 className="font-bold text-3xl">Network Configurations</h1>

        <div className="grid grid-cols-12 gap-3 justify-items-center">
          {/* Left column */}
          <div className="col-span-5 w-full min-w-3xs space-y-8">
            {/* Overview panel */}
            <div className="bg-gray-200 p-6 rounded-2xl shadow-lg">
              <h2 className={"font-bold text-2xl mb-6"}>Network Overview</h2>
              <div className={"flex flex-row gap-2.5"}>
                {/* Network logo */}
                <img
                  src={wifiIcon}
                  alt="Wi-Fi icon"
                  className="w-27 h-27 shadow-2xl rounded-full"
                />
                <a
                  href="https://www.flaticon.com/free-icons/ui"
                  title="ui icons"
                >
                  <span className={"sr-only"}>
                    Ui icons created by juicy_fish - Flaticon
                  </span>
                </a>
                <div className={"flex flex-col gap-2.5 text-lg"}>
                  <p className={""}>
                    <span class={"font-bold"}>Connection Status: </span>
                    {signal}
                  </p>
                  <p className={""}>
                    <span class={"font-bold"}>External Wi-Fi: </span>
                    {externalSsid}
                  </p>
                  <p className={""}>
                    <span class={"font-bold"}>Device Wi-Fi: </span>
                    {apSsid}
                  </p>
                </div>
              </div>
            </div>

            {/* Scan networks panel */}
            <div className="bg-gray-200 p-6 rounded-2xl shadow-lg">
              {isLoading ? (
                <LoadingSpinner size={10} label="Scanning for networks..." />
              ) : (
                <div className="flex flex-col items-center">
                  {/* Centered vertical layout */}
                  <h1 className="font-bold text-2xl mb-6">
                    Available Networks
                  </h1>

                  {/* Scan network button */}
                  <div className="pt-4">
                    <button className={buttonStyle} onClick={getNetworks}>
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
                </div>
              )}
            </div>
          </div>

          {/* Right column */}
          <div className="col-span-7 w-full">
            {/* Network settings panel */}
            <div className={"bg-gray-200 p-6 rounded-2xl shadow-lg space-y-6"}>
              <h2 className={"font-bold text-2xl mb-6"}>Network Settings</h2>
              <DeviceSettings
                apSsid={apSsid}
                apPassword={apPassword}
                externalSsid={externalSsid}
              />
              <div
                className={`flex flex-col ${showEnterWifiPassword ? "block" : "hidden"}`}
              >
                {/* Form for entering password */}
                <p className="text-lg font-bold mt-4" id={"passwordForm"}>
                  Selected Network: {selectedNetwork}
                </p>
                <TextEntry
                  label="Password"
                  entryType="password"
                  presetText="Enter Password"
                  onChange={setPassword}
                />
                <p
                  className={`font-bold text-red-700 ${connectionError === "" ? "hidden" : "block"}`}
                >
                  {connectionError}
                </p>

                <button
                  className={`${buttonStyle} mt-4 max-w-20`}
                  onClick={handleNetworkRequest}
                >
                  Submit
                </button>
              </div>
            </div>
          </div>
        </div>
      </div>
    </>
  );
};

export default NetworkPage;
