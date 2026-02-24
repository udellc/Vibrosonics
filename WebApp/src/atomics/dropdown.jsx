/***************************************************************
 * File: dropdown.jsx
 *
 * Date: 02/19/2026
 *
 * Description: UI dropdown componene
 *
 * Author: Bella Mann
 ***************************************************************/

import { useState } from "preact/hooks";

/**
 * @brief The Checkbox component is a skeleton for an audio analysis setting which uses checkbox
 *        for reconfiguration
 *
 * @param {Object} props - Expanded object for the checkbox settings
 * @param {String} props.label - Text to display next to checkbox
 * @param {Array} props.options - Initial/Default value
 * @param {function} props.onChange - Callback that receives boolean state
 */

export default function DropDown({ label = "Select Option", options = [], onChange}) {
    const [isOpen, setIsOpen] = useState(false);
    const [selected, setSelected] = useState(null);

    const toggleDropdown = () => setIsOpen(!isOpen);

    const handleChange = (e) => {
        setSelected(e);
        setIsOpen(false);
        if (onChange) onChange(e);
    };

    return (
        <div className="relative inline-block text-left w-32">
            <button
                onClick={toggleDropdown}
                className="flex items-center justify-between w-full px-4 py-2 bg-gray-200 rounded-xl shadow-inner hover:bg-gray-300 transition-colors"
            >
                <span>{selected || label}</span>
                <svg className={`w-5 h-5 transition-transform ${isOpen ? 'rotate-180' : ''}`} fill="none" stroke="currentColor" viewBox="0 0 24 24">
                    <path strokeLinecap="round" strokeLinejoin="round" strokeWidth="2" d="M19 9l-7 7-7-7"/>
                </svg>
            </button>

            {isOpen && (
                <div className="absolute z-10 w-full mt-2 bg-white border border-gray-200 rounded-xl shadow-lg overflow-hidden">
                    <ul className="py-1">
                        {options.map((option, index) => (
                            <li key={index}>
                                <button
                                    onClick={() => handleChange(option)}
                                    className="w-full text-left px-4 py-2 hover:bg-blue-500 hover:text-white transition-colors"
                                    >
                                        {option}
                                </button>
                            </li>
                        ))}
                    </ul>
                </div>
            )}
        </div>
    );
}
