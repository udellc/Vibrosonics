/***************************************************************
 * File: radioPage.jsx
 *
 * Date: 03/11/2026
 *
 * Description: FM Radio page setup
 *
 * Author: Bella Mann
 ***************************************************************/
// @ts-ignore
import { useRef, useEffect, useState } from "preact/hooks";

// Make sure this path matches where your textEntry.jsx file is saved
// @ts-ignore
import TextEntry from '../components/textEntry'; 

const PRESET_STATIONS = [
    { name: "BBC Radio 1", url: "http://stream.live.vc.bbcmedia.co.uk/bbc_radio_one" },
    { name: "Dance Wave Retro", url: "http://dancewave.online/retro.mp3" },
    { name: "Lofi Chill", url: "http://stream.zeno.fm/f3wvbbqmdg8uv" },
    { name: "Classic Rock", url: "http://streaming.exclusive.radio/er/classicrock/icecast.audio" }
];

const RadioPage = () => {
    // @ts-ignore
    const [input, setInput] = useState('');
    const [isOpen, setIsOpen] = useState(false);

    /**@type {[any, Function]} */
    const [selectedStation, setSelectedStation] = useState(null)

    // @ts-ignore
    const handleInputChange = (newText) => {
        setInput(newText);
        alert("success");
    }

    // @ts-ignore
    const handleSelect = (station) => {
        setSelectedStation(station);
        setIsOpen(false);
    }

    return (
        <div className="flex flex-col min-h-[80vh] items-center justify-center gap-4"> 
            <h2 className="text-xl font-bold">FM Radio</h2>
            
            <div className="flex items-center justify-center">
                <label className="pr-2">Input a Station:</label>
                <input 
                    id="Station" 
                    type="number" 
                    className="border rounded-xl px-2 hover:border-amber-500 "
                    step="any"></input>
            </div>

            <label className="font-bold"> OR </label>

            <div className="relative w-full max-w-xs flex items-center">
                <label className="block mb-2 w-40">
                    Choose a Station:
                </label>
                <div
                    onClick={() => setIsOpen(!isOpen)}
                    className="w-50 flex items-center justify-between border rounded-xl px-4 py-3 bg-white cursor-pointer hover:border-amber-500 transition-colors"
                >
                    <span className={selectedStation ? "test-black" : "text-gray-400"}>
                        {selectedStation ? selectedStation.name : "Select a station.."}
                    </span>
                    <span className="text-sm">
                        {isOpen ? '▲' : '▼'}
                    </span>
                </div>

                {isOpen && (
                    <div className="absolute top-full left-0 w-full mt-2 bg-white border border-gray-200 rounded-xl shadow-lg z-50 overflow-hidden">
                        {PRESET_STATIONS.map((station, index) => (
                            <div 
                                key={index}
                                onClick={() => handleSelect(station)}
                                className={`px-4 py-3 cursor-pointer transition-colors ${
                                    selectedStation?.name === station.name 
                                        ? 'bg-amber-100 font-semibold' 
                                        : 'hover:bg-gray-50'
                                }`}
                            >
                                {station.name}
                            </div>
                        ))}
                    </div>
                )}
            
            </div>
            
            <button className="bg-amber-200 border border-amber-600 hover:bg-amber-600 rounded-xl px-4"
                onClick={handleInputChange}
            >Submit</button>
        </div>
    );
}

export default RadioPage;
