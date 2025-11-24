/***************************************************************
 * File: eqSettings.jsx
 *
 * Date: 11/24/2025
 *
 * Description: define presets for the different EQ settings
 *
 * Author: Bella Mann
 ***************************************************************/


//TODO: change EQ presets based on libraries?
export const EQ_PRESETS = {
    Rock: [
      { id: 'bass', title: 'Bass', min:0, max:100},
      { id: 'mid', title: 'Mid', min:0, max:10},
      { id: 'treb', title: 'Treble', min:0, max:10},
      { id: 'pres', title: 'Presence', min:0, max:10},
    ],
    EDM: [
      { id: 'bass', title: 'Bass', min:0, max:100},
      { id: 'treble', title: 'Highs', min:0, max:10},
      { id: 'sub', title: 'Sub-Bass', min:0, max:10},
    ],
    Country: [
      { id: 'bass', title: 'Bass', min:0, max:100},
      { id: 'mid', title: 'Mid', min: 0, max:100},
      { id: 'twang', title: 'Twang', min:0, max:10} 
    ],
    Jazz: [
      { id: 'low', title: 'Low', min:0, max:100},
      { id: 'mid', title: 'Mid', min: 0, max:100},
      { id: 'high', title: 'High', min:0, max:10} 
    ]
}