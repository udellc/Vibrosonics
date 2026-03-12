/***************************************************************
 * File: radioPage.jsx
 *
 * Date: 03/11/2026
 *
 * Description: FM Radio page setup
 *
 * Author: Bella Mann
 ***************************************************************/
import { route } from "preact-router";
import { useState, useEffect } from "preact/hooks";
import { api } from "../utils/utils";
import InfoButton from "../atomics/infoButton";
import ModuleDisplay from "../data/moduleDisplay.json";
import { FREQUENCY_MAPPING, MODULE_TYPE, WAVE_TYPE } from "../utils/utils";

// Make sure this path matches where your textEntry.jsx file is saved
import TextEntry from '../components/textEntry'; 

const MyForm = () => {
  
  };

const RadioPage = () => {
    const [input, setInput] = useState('');

    const handleInputChange = (newText) => {
        setInput(newText);
    }

    return (
        <div className="p-8 items-center justify-center">
            <h2 className="text-xl font-bold bordermb-4">Radio Station</h2>
            
            <TextEntry 
                label="Station Number" 
                entryType="text" 
                presetText="00.00" 
                onChange={handleInputChange} 
            />

            {/* testing to show that the state is updating 
            <p className="mt-4 text-gray-500">value: {input}</p>*/}
        </div>
    );
}

export default RadioPage;
