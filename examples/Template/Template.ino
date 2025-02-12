/**
 * @file template.ino
 *
 * This example serves a reference point for creating your own Vibrosonics
 * programs.
 */

#include "VibrosonicsAPI.h"

/**
 * Instance of the Vibrosonics API. Core to all operations including:
 * -- FFT operations with ArduinoFFT
 * -- Audio spectrum storage with the spectrogram
 * -- Audio input and synthesis through AudioLab
 * -- AudioPrism module management and analysis
 */
VibrosonicsAPI vapi = VibrosonicsAPI();

/**
 * It is recommended to create AudioPrism modules here, along with their
 * corresponding grains if applicable.
 *
 * For example:
 */
// Creates major peaks module to analyze 4 peaks
MajorPeaks mp = MajorPeaks();
//Creates a grain array with 4 grains on channel 0 with the SINE wave type
Grain* mpGrains = vapi.createGrainArray(4, 0, SINE);

/**
 * Runs once on ESP32 startup.
 * VibrosonicsAPI initializes AudioLab and adds modules to be managed
 * Grain shaping should also be configured here.
 */
void setup()
{
  Serial.begin(115200);
  vapi.init();
  // Add any AudioPrism modules to the API here
  vapi.addModule(&mp);
  // Shape the MajorPeaks grains
  vapi.setGrainAttack(mpGrains, 4, 0, 0, 2);
  vapi.setGrainSustain(mpGrains, 4, 0, 0, 1);
  vapi.setGrainRelease(mpGrains, 4, 0, 0, 4);

}

/**
 * Main loop for analysis and synthesis.
 * Has 5 key steps:
 * 1. Check if AudioLab is ready
 * 2. Process input
 * 3. Analyze processed data
 * 4. Generate waves with grains or through the API
 * 5. Synthesize and output waves through AudioLab
 */

void loop()
{
  // First check to make sure that the AudioLab input buffer has 
  // been filled
  if (!AudioLab.ready()) {
    return;
  }

  // Second, the input must be processed for analysis
  // Steps:
  // Copies samples from the AudioLab buffer to the vReal array
  // Perfroms FFT operations on vReal
  // Pushes vReal to the spectrogram
  vapi.processInput();
  
  // Third, analyze the data with the added AudioPrism modules
  vapi.analyze();

  // Fourth, generate waves
  // NOTE: This can be done through grains or by assigning waves with
  //       the API. Both are demonstrated below

  // For modules you will need to get their output to perform further manipulation
  float** mpData = mp.getOutput();

  // Grains method
  // With frequency and amplitude data you will want to trigger the grains
  vapi.triggerGrains(mpGrains, 4, mpData);

  // Direct wave creation
  vapi.assignWaves(mpData[0], mpData[1], 4, 0);
  // Perform any additional manipulation that you want here

  // Update grains if you are using them
  vapi.updateGrains();
  // Lastly, synthesize all created waves through AudioLab
  AudioLab.synthesize();
  // Uncomment for debugging:
  // AudioLab.printWaves();
}
