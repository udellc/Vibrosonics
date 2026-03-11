/***************************************************************
 * File: infoButton.jsx
 *
 * Date: 03/10/2026
 *
 * Description: UI infoButton componenet
 *
 * Author: Bella Mann
 ***************************************************************/

/**
 * @brief Info Button Component
 * @param {Object} setting - Expanded object for the checkbox settings
 * @param {function(Event): void} setting.onClick - function to run when clicked
 */

export default function InfoButton({ onClick }) {
    return (
        <button
            type="button"
            onClick={onClick}
            className="flex items-center justify-center w-6 h-6 border border-gray-300 rounded cursor-pointer bg-w"
        >
            <svg 
                //xmlns="http://www.w3.org/2000/svg" 
                width="16" 
                height="16" 
                viewBox="0 0 24 24" 
                fill="none" 
                stroke="black" 
                strokeWidth="2" 
                strokeLinecap="round" 
                strokeLinejoin="round"
            >
                <circle cx="12" cy="12" r="10"></circle>
                <line x1="12" y1="16" x2="12" y2="12"></line>
                <circle cx="12" cy="8" r="1" fill="black" stroke="none"></circle>
            </svg>
        </button>
    )
}
