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
 * @param { Object } _ - Expanded object for the network card params
 * @param { String } _.SSID - Network SSID to be displayed
 * @param { CallableFunction } _.onConnect - Function that gets invoked when the connect button is clicked
 *
 * @returns UI component for a network card
 */
const NetworkCard = ({ SSID, onConnect }) => {
  return (
    <div className="flex flex-row p-3">
      <p>{SSID}</p>
      <button
        className="border-amber-500 border-2 cursor-pointer ml-4"
        onClick={() => onConnect()}
      >
        Connect
      </button>
    </div>
  );
};

export default NetworkCard;
