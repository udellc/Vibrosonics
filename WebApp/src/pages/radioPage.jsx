/***************************************************************
 * File: radioPage.jsx
 *
 * Date: 03/11/2026
 *
 * Description: FM Radio page setup
 *
 * Author: Bella Mann
 ***************************************************************/
import { useState, useEffect } from "preact/hooks";

// Make sure this path matches where your textEntry.jsx file is saved
import TextEntry from '../components/textEntry'; 

const RadioPage = () => {
    const [input, setInput] = useState('');

    const handleInputChange = (newText) => {
        setInput(newText);
    }

    return (
        <div className="flex flex-col min-h-[80vh] items-center justify-center gap-4"> 
            <h2 className="text-xl font-bold">FM Radio</h2>
            
            <div className="flex items-center justify-center">
                <TextEntry 
                    label="Station Number" 
                    entryType="text" 
                    presetText="00.00" 
                    onChange={handleInputChange} 
                />
            </div>
        </div>
    );
}

export default RadioPage;
