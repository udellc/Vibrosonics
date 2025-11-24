/***************************************************************
 * File: networkCard.jsx
 * 
 * Date: 11/22/2025
 * 
 * Description: This is the component for the networking page
 * which displays a scanned network, showing the SSID and a
 * connect button.
 * 
 * Author: Ivan Wong
 ***************************************************************/

/**
 * @brief Network card is a small component that displays the given SSID with a custom callback
 * when the connect button is clicked.
 * 
 * @param {Object} params - Expanded object for the network card params
 * @param {String} params.SSID - Network SSID to be displayed
 * @param {CallableFunction} params.onConnect - Function that gets invoked when the connect button is clicked
 * 
 * @returns UI component for a network card
 */
const NetworkCard = ({ SSID, onConnect }) => {
  return (
    <div className="flex flex-row p-3">
      <p>
        {SSID}
      </p>
      <div className="w-4"></div>
      <button className="border-amber-500 border-2 cursor-pointer"
        onClick={() => onConnect()}>
        Connect
      </button>
    </div>
  );
}

export default NetworkCard;
