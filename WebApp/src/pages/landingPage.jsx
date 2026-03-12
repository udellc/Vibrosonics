/***************************************************************
 * File: landingPage.jsx
 *
 * Date: 11/06/2025
 *
 * Description: The landing page for web app
 *
 * Author: Ivan Wong
 ***************************************************************/

import { route } from "preact-router";
import { useState, useEffect } from "preact/hooks";
import { api } from "../utils/utils";
import InfoButton from "../atomics/infoButton";

/**
 * @brief Displays the landing page for the Vibrosonics web app
 */
const LandingPage = () => {
  const [isAudioSettingBtnVisible, setAudioSettingBtnVisible] = useState(false);

  /**
   * @brief Gets the network SSID on mount, making the audio settings button visible if not connected to AP mode
   */
  useEffect( () => {
    const checkNetwork = async () => {
      const ssid = await api("GET", "/network/getSsid");

      if (ssid.data !== "Vibrosonics-Unsecure") {
        setAudioSettingBtnVisible(true);
      }
    };
    checkNetwork();
  }, []);

  const handleInfoClick = () => {
    // TODO: change to pop up rather than alert
    alert("working!");
  };

  return (
    <div className="mt-20 ml-10 mr-10 min-h-[60vh]">
      {/* Vertical layout */}
      <div className="flex flex-col">
        <h2 className="text-4xl font-bold mb-10">Welcome</h2>
        <p className="text-2xl">Feel the Music, Your Way</p>

        {/* Horizontal layout */}
        <div className="flex flex-row items-center">
          <div className="flex flex-col justify-between max-w-[50%] mr-6">
            <p className="mb-4">
              VibroSonics is dedicated to making music and sound accessible to
              the hearing impaired community through the power of touch. We
              believe that the experience of music is universal, and out
              technology translates sound into nuanced vibrations you can feel.
            </p>
            <p>
              Our platform allows you to take full control of your sensory
              experiences. Select any song and our technology converts it into
              rich, tactile sensors. You can precisely adjust the frequency to
              find the vibration range that resonates with you and control the
              gain to set the perfect intensity, from a subtle pulse to a
              powerful beat.
            </p>
          </div>
          <div className="flex flex-col gap-10">
            <button
              className="p-3 bg-[#fcd34d] border border-[#ccc] rounded-lg cursor-pointer font-bold shadow-sm hover:bg-[#fbbf24]"
              onClick={() => route("/network", false)}
            >
              Connect to Network
            </button>
            
            {/*<InfoButton onClick={handleInfoClick} /> REMOVE WHEN DONE TESTING*/}
            <button
              className={`p-3 bg-[#fcd34d] border border-[#ccc] rounded-lg cursor-pointer font-bold shadow-sm hover:bg-[#fbbf24] ${isAudioSettingBtnVisible ? "visible" : "invisible"}`}
              onClick={() => route("/modules", false)}
            >
              Adjust Audio Settings
            </button>
          </div>

          
        </div>

        <div className="grid grid-cols-1 md:grid-cols-3 gap-8 p-10 pt-24 max-w-6xl mx-auto">
          {/** info bubble 1 */}
          <div className="flex flew-row gap-4">
            <InfoButton onClick={() => {}}/>
            <h3 className="text-xl font-bold text-gray-800">Audio Haptics</h3>
            <p className="text-gray-500 mt-2 leading-relaxed">
              Audio haptics technology enhances sensory perception for individuals 
              who are deaf or hard of hearing by integrating auditory signals with
              tactile feedback.
            </p>
          </div>

          {/** info bubble 2 */}
          <div className="flex flew-row gap-4">
            <InfoButton onClick={() => {}}/>
            <h3 className="text-xl font-bold text-gray-800">The Interface</h3>
            <p className="text-gray-500 mt-2 leading-relaxed">
              Our intuitive interface seamlessly unites he audio haptic device 
              with an interactive software experience, delivering a fluid and 
              engaging way for users to feel and control their music.
            </p>
          </div>

          {/** info bubble 3 */}
          <div className="flex flew-row gap-4">
            <InfoButton onClick={() => {}}/>
            <h3 className="text-xl font-bold text-gray-800">The Interface</h3>
            <p className="text-gray-500 mt-2 leading-relaxed">
              Led by Dr. Chet Udell of Oregon State University,
              the VibroSonics team directs a dedicated research 
              group of students and is supported by a strategic 
              partnership with CymaSpace
            </p>
          </div>
        </div>
      </div>
    </div>
  );
};

export default LandingPage;
