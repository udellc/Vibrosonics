
/**
 * @file Grains.ino
 *
 * This example serves a reference point for using granular synthesis
 * through grains.
 * It demonstrates a frequency and amplitude sweep, duration changes,
 * wave shape variations, and how each parameter functions in isolation.
 */

#include "VibrosonicsAPI.h"

/**
 * BACKGROUND: What is a Grain?
 * In a nutshell, grains are tiny "slices" of audio from a larger sample.
 * By subdividing a sample into individual grains, more sophisticated 
 * analysis can be performed. For example, layering multiple grains on
 * top of each other can create new soundscapes, or a particular instrument
 * can be isolated and played over a long duration. Note that due to the
 * fine tuned nature of granular synthesis, applying generalizations will
 * often create "glitchy" sounds, so users will likely need to play around
 * and experiment to achieve desired results.
 *
 * In Vibrosonics, grains are a layer on top of AudioLab waves to provide
 * some additional features. These features are as follows:
 * -- Duration modifications: Grains can be configured to run over many windows
 *    in a flexible way, whereas AudioLab waves only run for 1 window
 * -- Frequency and Amplitude modulation: Grains give the user the power to
 *    modulate frequencies and amplitudes on top of existing analysis to
 *    do things like make certain events stand out more for example.
 * -- Wave Shaping: Through the Attack, Sustain, and Release states, grains
 *    give the user the ability to create custom curves and wave shapes on
 *    top of their base waveshape from AudioLab (SINE, COSINE, SQUARE, SAWTOOTH,
 *    TRIANGLE)
 *
 * NOTES:
 * In our testing, we found that grains are a bit more of a niche feature
 * rather than being ideal as the primary synthesis method. A good
 * example of an ideal situation for using grains are snare drums.
 * Any instrument/vocal/tone with a long lifetime is a good canidate
 * for using Grains.
 * This functionality is demoed in
 * examples/AdvancedGrains/AdvancedGrains.ino
 */

/**
 * Instance of the Vibrosonics API. Core to all operations including:
 * -- FFT operations with ArduinoFFT
 * -- Audio spectrum storage with the spectrogram
 * -- Audio input and synthesis through AudioLab
 * -- AudioPrism module management and analysis
 * -- Grain creation and management
 */
VibrosonicsAPI vapi = VibrosonicsAPI();

// Creates major peaks module to analyze 4 peaks
MajorPeaks mp = MajorPeaks();
//Creates a grain array with 4 grains on channel 0 with the SINE wave type
Grain* mpGrains = vapi.createGrainArray(4, 0, SINE);
Grain* freqSweepGrains = vapi.createGrain(1, 0, SINE);

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
  if (!AudioLab.ready()) {
    return;
  }

  // process the input buffer
  //vapi.processInput();

  // Run module analysis
  //vapi.analyze();

  // For modules you will need to get their output to perform further manipulation
  //float** mpData = mp.getOutput();

  // Grains method
  // With frequency and amplitude data you will want to trigger the grains
  //vapi.triggerGrains(mpGrains, 4, mpData);

  // Update grains if you are using them
  vapi.updateGrains();
  // Lastly, synthesize all created waves through AudioLab
  AudioLab.synthesize();
  // Uncomment for debugging:
  // AudioLab.printWaves();
}
