/***************************************************************
 * File: deviceSettings.jsx
 *
 * Date: 5/23/2026
 *
 * Description: UI component for handling the on-device
 * networking settings.
 *
 * Author: Ivan Wong
 ***************************************************************/

import TextEntry from "../../atomics/textEntry";
import { useState } from "preact/hooks";
import lockIcon from "../../../assets/padlock.png";
import { api, HTTP_STATUS } from "../../utils/utils";

const buttonStyle =
  "p-2 bg-amber-200 border border-amber-500 rounded-lg cursor-pointer hover:bg-[#fbbf24]";

/**
 * @brief UI Component for handling network device settings
 *
 * @param {Object} _
 * @param {String} _.apSsid - AP SSID to display
 * @param {String} _.apPassword - Password to reveal
 * @param {String} _.externalSsid - External Ssid to display
 */
const DeviceSettings = ({ apSsid, apPassword, externalSsid }) => {
  const [revealPassword, setRevealPassword] = useState(false);
  const [newApSsid, setNewApSsid] = useState("");
  const [newApPassword, setNewApPassword] = useState("");

  /**
   * @brief API handler for saving access point settings to the ESP32
   */
  const handleSaveSettings = async () => {
    // // WiFi WPA2 requires at least 8 chars for a password 
    if (newApPassword.length < 8) {
      window.alert("Warning: Password must be at least 8 characters.");
      return;
    }
    if (window.confirm("Warning: This Will Overwrite Current Settings. Continue?")) {
      const data = {
        newApSsid,
        newApPassword
      };
      const res = await api("PATCH", "/network/saveAPSettings", data);
      const status = res?.status;
      
      if (status === HTTP_STATUS.OK) {
        window.alert("Restart Device to Apply Changes.");
      } else if (status === HTTP_STATUS.UNPROCESSABLE) {
        window.alert("Invalid Credentials.");
      } else {
        console.error("Could Not Save Device Settings.");
      }
    }
  };

  /**
   * @bried API handler for forgetting and disconnecting the external WiFi source
   */
  const handleForgetWifi = async () => {
    if (externalSsid === "N/A") {
      window.alert("Not Connected to External Wi-Fi");
      return;
    }

    if (window.confirm("Forget External Wi-Fi?")) {
        const res = await api("PATCH", "/network/forgetWifi")

      if (res?.status === HTTP_STATUS.OK) {
        window.alert("Restart Device to Apply Changes.");
      }
    }
  };

  /**
   * @brief API handler for resetting all network configs to factory settings
   */
  const handleReset = async () => {
    if (window.confirm("Restore Factory Settings?")) {
      const res = await api("PATCH", "/network/resetSettings");
      
      if (res?.status === HTTP_STATUS.OK) {

        window.alert("Restart Device to Apply Changes.");
      }
    }
  };

  return (
    <div className="flex flex-col space-y-3" id="device-network">
      <h3 className={"space-y-4 font-bold text-xl gap-2.5 mb-4"}>
        On-Device WiFi Settings
      </h3>
      <div className={"flex flex-row"}>
        <img
          src={lockIcon}
          alt="Lock icon"
          className="w-27 h-27 shadow-2xl rounded-full mr-4"
        />
        <a
          href="https://www.flaticon.com/free-icons/password"
          title="password icons"
        >
          <span className={"sr-only"}>
            Password icons created by heisenberg_jr - Flaticon
          </span>
        </a>
        <div className={"flex flex-col gap-2"}>
          <TextEntry
            label={"AP SSID"}
            entryType={"text"}
            presetText={apSsid}
            onChange={setNewApSsid}
          />
          <TextEntry
            label={"AP Password"}
            entryType={revealPassword === true ? "text" : "password"}
            presetText={revealPassword === true ? apPassword : "Hidden"}
            onChange={setNewApPassword}
          />
          <div className="flex flex-row space-x-4">
            <button
              className={buttonStyle}
              onClick={() => setRevealPassword(!revealPassword)}
            >
              {revealPassword ? "Hide" : "Reveal"} Password
            </button>
            <button className={buttonStyle} onClick={handleSaveSettings}>
              Save Settings
            </button>
          </div>
        </div>
      </div>
      <div>
        <h3 className={"font-bold text-xl mb-2"}>External Wi-Fi Settings</h3>
        <button className={buttonStyle} onClick={handleForgetWifi}>
          Forget External Wi-Fi
        </button>
      </div>
      <div>
        <h3 className={"font-bold text-xl mb-2"}>Reset Network Settings</h3>
        <button className={buttonStyle} onClick={handleReset}>
          Reset All Wi-Fi Settings
        </button>
      </div>
    </div>
  );
};

export default DeviceSettings;
