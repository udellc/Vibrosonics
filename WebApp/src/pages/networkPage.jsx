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
import TextEntry from "../atomics/textEntry";
import LargeRadioButton from "../atomics/largeRadioButton";
import wifiIcon from "../../assets/wifi.png";
import lockIcon from "../../assets/padlock.png";
import LoadingSpinner from "../atomics/loadingSpinner";

const buttonStyle = "p-2 bg-amber-200 border border-amber-500 rounded-lg cursor-pointer hover:bg-[#fbbf24]";

/**
 * @brief Defines the network page components, allowing a user to view and connect to different networks or adjust
 * network settings.
 */
const NetworkPage = () => {
  const [availableNetworks, setAvailableNetworks] = useState(["example"]);
  const [isLoading, setIsLoading] = useState(false);
  const [selectedNetwork, setSelectedNetwork] = useState("");
  const [password, setPassword] = useState("");
  const [showEnterWifiPassword, setTextForm] = useState(false);
  const [isConnecting, setIsConnecting] = useState(false);

  // On-device network info
  const [currentSsid, setCurrentSsid] = useState("");
  const [currentMode, setCurrentMode] = useState("extern");

  /**
   * @brief Makes a request to the ESP32 to scan and return available networks
   */
  const getNetworks = async () => {
    // setConnectionErrorText("");
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
    try {
      setIsConnecting(true);
      const res = await api("POST", "/network/connect", payload);

      if (res?.status == HTTP_STATUS.ACCEPTED) {
        // TODO: update some sort of context that the header/footer use to show disconnect option
        route("/modules", true);
      }
    } catch (error) {
      console.error("Failed to connect to network", error);
    } finally {
      setIsConnecting(false);
    }
  };

  /**
   * @brief Gets the name of current Network SSID
   *
   * TODO: change to getNetworkInfo
   */
  const getNetworkSsid = async () => {
    const res = await api("GET", "/network/getSsid");

    if (res?.status == HTTP_STATUS.OK) {
      setCurrentSsid(res?.data);
    } else {
      setCurrentSsid("Unable to retrieve SSID");
    }
  };

  // Scan for networks and get current network name on mount
  useEffect(() => {
    getNetworkSsid();
    getNetworks();
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
            <div className={"w-xs h-30 bg-gray-300 rounded-2xl border-2 border-black p-5"}>
              <LoadingSpinner size={10} label={`Connecting To: ${selectedNetwork}`} />
            </div>
          </div>
        </>
      ) : (
        <></>
      )}
      <div
        className="mt-16 min-h-[60vh] mx-8"

        // Disables tabbing into the network components
        inert={isConnecting ? true : false}
        aria-hidden={isConnecting}
      >
        <h1 className="font-bold text-3xl">Network Configurations</h1>

        <div className="grid grid-cols-12 gap-3 justify-items-center">
          {/* Left column */}
          <div className="col-span-5 w-full space-y-8">
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
                    <span class={"font-bold"}>Status: </span> TODO
                  </p>
                  <p className={""}>
                    <span class={"font-bold"}>Current Network: </span>
                    {currentSsid}
                  </p>
                  <p className={""}>
                    <span class={"font-bold"}>IP Address: </span>TODO
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
                    <button
                      className={buttonStyle}
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

                  {showEnterWifiPassword ? (
                    <>
                      {/* Form for entering password */}
                      <p className="mt-4">
                        Selected Network: {selectedNetwork}
                      </p>
                      <TextEntry
                        label="Password"
                        entryType="password"
                        presetText="Enter Password"
                        onChange={setPassword}
                      />
                      <button
                        className={buttonStyle}
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
          </div>

          {/* Right column */}
          <div className="col-span-7 w-full">

            {/* Network settings panel */}
            <div className={"bg-gray-200 p-6 rounded-2xl shadow-lg space-y-6"}>
              <h2 className={"font-bold text-2xl mb-6"}>Network Settings</h2>

              <div className={"space-y-4"}>
                <h3 className={"font-bold text-xl"}>Select Network Mode</h3>
                <LargeRadioButton
                  label="Use External Wi-Fi (Station Mode)"
                  description="Connect to an existing Wi-Fi network."
                  value="external"
                  checked={currentMode === "external"}
                  onChange={setCurrentMode}
                />
                <LargeRadioButton
                  label="Use Device AP Mode (Access Point)"
                  description="Device creates its own Wi-Fi network."
                  value="ap"
                  checked={currentMode === "ap"}
                  onChange={setCurrentMode}
                />
              </div>
              <div>
                <h3 className={"font-bold text-xl gap-2.5 mb-4"}>
                  On-Device WiFi Settings
                </h3>
                <div className={"flex flex-row"}>
                  <img src={lockIcon} alt="Lock icon" className="w-27 h-27 shadow-2xl rounded-full mr-4"/>
                  <a href="https://www.flaticon.com/free-icons/password" title="password icons">
                    <span className={"sr-only"}>
                      Password icons created by heisenberg_jr - Flaticon
                    </span>
                  </a>
                  <div className={"flex flex-col gap-2"}>
                    <TextEntry
                      label={"AP SSID"}
                      entryType={"text"}
                      presetText={currentSsid}
                      onChange={() => console.log("s")}
                    />
                    <TextEntry
                      label={"AP Password"}
                      entryType={"password"}
                      presetText={"Hidden"}
                      onChange={() => console.log("s")}
                    />
                    <button className={buttonStyle}
                      onClick={() => console.log("handle reveal pw")}
                    >
                      Reveal Password
                    </button>
                  </div>
                </div>
              </div>

              <div>
                <h3 className={"font-bold text-xl mb-2"}>
                  Forget External Wi-Fi Settings
                </h3>
                <button className={buttonStyle}
                  onClick={() => console.log("handle forget wifi")}
                >
                  Forget External Wi-Fi
                </button>
              </div>

              <div className={"relative"}>
                <h3 className={"font-bold text-xl mb-2"}>
                  Reset Network Settings
                </h3>
                <button className={buttonStyle}
                  onClick={() => console.log("handle reset")}
                >
                  Reset
                </button>
                <button className={`absolute bottom-0 right-0 ${buttonStyle}`}
                  onClick={() => console.log("handle submit settings")}
                >Submit Settings</button>
              </div>
            </div>

            
          </div>
        </div>
      </div>
    </>
  );
};

export default NetworkPage;
