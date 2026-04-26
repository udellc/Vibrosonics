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
 * @param {Object} props - Expanded object for component settings
 * @param {Object} props.infoText
 * @param {Boolean} props.showToolTip
 * @param {function(Event): void} props.onClick - function to run when clicked
 */

export default function InfoButton({ onClick, infoText, showToolTip = true }) {
    return (
        <div className="relative flex flex-col items-center group">
            <button
                type="button"
                onClick={onClick}
                className="flex items-center justify-center w-6 h-6 rounded cursor-pointer bg-w"
            >
                <svg 
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

            {showToolTip && (
                <div className="absolute bottom-full mb-2 w-max max-w-xs px-3 py-2 bg-gray-400 text-black text-xs text-center rounded-lg shadow-lg 
                                opacity-0 invisible group-hover:opacity-100 group-hover:visible transition-all duration-200 z-50 pointer-events-none">
                    {infoText}
                    <div className="absolute top-full left-1/2 -translate-x-1/2 border-4 border-transparent border-t-gray-400"></div>
                </div>
            )}
        </div>
    )
}
