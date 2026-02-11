/***************************************************************
 * File: checkbox.jsx
 *
 * Date: 11/17/2025
 *
 * Description: UI checkbox component for advanced mode
 *
 * Author: Bella Mann
 ***************************************************************/

import { useState } from "preact/hooks";

/**
 * @brief The Checkbox component is a skeleton for an audio analysis setting which uses checkbox
 *        for reconfiguration
 *
 * @param {Object} setting - Expanded object for the checkbox settings
 * @param {String} setting.label - Text to display next to checkbox
 * @param {Number} [setting.initialValue] - Initial/Default value
 * @param {function(string, boolean): void} setting.onChange - Callback that receives boolean state
 */

export default function Checkbox({ 
    label, 
    initialValue = 0, 
    onChange 
}) {
    const [checked, setChecked] = useState(false);

    const handleChange = (e) => {
        const isChecked = e.target.checked;
        setChecked(isChecked);
        if(onChange)
            onChange(label, isChecked);
    };

    return (
        <label className="flex items-center gap-2 cursor-pointer">
            <input
                type="checkbox"
                checked={checked}
                onInput={handleChange}
                className="w-4 h-4"
            />
            <span>{label}</span>
        </label>
    )
}
