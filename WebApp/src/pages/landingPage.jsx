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

/**
 * @brief Displays the landing page for the Vibrosonics web app
 */
const LandingPage = () => {
  const handleConnectNetwork = () => {
    // Redirect to the network page, but do not replace the prev content yet
    route("/network", false);
  };

  return (
    <div className="mt-20 ml-10 mr-10">
      {/* Vertical layout */}
      <div className="flex flex-col">
        <h2 className="text-4xl font-bold mb-10">Welcome</h2>
        <p className="text-2xl">Feel the Music, Your Way</p>

        {/* Horizontal layout */}
        <div className="flex flex-row">
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
          <button
            className="border-2 border-amber-500 cursor-pointer max-h-[30px]"
            onClick={handleConnectNetwork}
          >
            Connect to Network
          </button>
        </div>
      </div>
    </div>
  );
};

export default LandingPage;
