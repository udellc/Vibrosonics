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
        //alert("success"); //FOR TESTING
    }

    return (
        <div className="flex flex-col min-h-[80vh] items-center justify-center gap-4"> 
            <h2 className="text-xl font-bold">FM Radio</h2>
            
            <div className="flex items-center justify-center">
                <label className="pr-2">Station</label>
                <input 
                    id="Station" 
                    type="number" 
                    className="border rounded-xl px-2" 
                    onChange={handleInputChange} 
                    step="any"></input>
            </div>
            <button className="bg-amber-200 border border-amber-600 rounded-xl px-4">Submit</button>
        </div>
    );
}

export default RadioPage;
