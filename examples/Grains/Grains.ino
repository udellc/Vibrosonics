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
 * Grains last for 15.6 * total duration milliseconds
 * This is due to the default sample rate of 8192 and window size of 128
 * 128/8192 = 15.6ms
 * Note that this will vary if you have messed with these settings in AudioLab
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

Grain* sweepGrain = vapi.createGrainArray(1, 0, SINE);

float targetFreq = 100.0;
/**
 * Runs once on ESP32 startup.
 * VibrosonicsAPI initializes AudioLab and adds modules to be managed
 * Grain shaping should also be configured here.
 */
void setup()
{
  Serial.begin(115200);
  vapi.init();
  /* Shape the frequency sweep grain
   * @param grains An array of grains
   * @param numGrains The size of a grain array
   * @param duration The number of windows the attack state will last for
   * @param freqMod The multiplicative modulation factor for the frequency of the attack state
   * @param ampMod The multiplicative modulation factor for the frequency of the attack state
   * @param curve Factor to influence the shave of the progression curve of the grain
   */
  vapi.shapeGrainAttack(sweepGrain, 1, 10, 1.0, 1.0, 1.0);
  vapi.shapeGrainSustain(sweepGrain, 1, 3, 1.0, 1.0);
  vapi.shapeGrainRelease(sweepGrain, 1, 8, 1.0, 1.0, 1.0);
  sweepGrain[0].setAttack(targetFreq, 0.5, sweepGrain[0].getAttackDuration());
  sweepGrain[0].setSustain(targetFreq, 0.5, sweepGrain[0].getSustainDuration());
  sweepGrain[0].setRelease(targetFreq, 0.0, sweepGrain[0].getReleaseDuration());
}

/**
 * Main loop for analysis and synthesis.
 */
void loop()
{
  if (!AudioLab.ready()) {
    return;
  }
  // Outside of frequency range, reset
  if(sweepGrain[0].getFrequency() >= 4000) targetFreq = 100.0;
  // Previous frequency has finished playing, increase grain frequency
  if(sweepGrain[0].getGrainState() == READY){
    targetFreq = targetFreq*1.05;
    sweepGrain[0].setAttack(targetFreq, 0.5, sweepGrain[0].getAttackDuration());
    sweepGrain[0].setSustain(targetFreq, 0.5, sweepGrain[0].getSustainDuration());
    sweepGrain[0].setRelease(targetFreq, 0.0, sweepGrain[0].getReleaseDuration());
  }
  // Progress the grain through its curve
  vapi.updateGrains();
  // Use AudioLab to synthesize waves for output
  AudioLab.synthesize();
  // Uncomment for debugging:
  Serial.printf("State: %i, Frequency: %f, Amplitude: %f\n", sweepGrain[0].getGrainState(), sweepGrain[0].getFrequency(), sweepGrain[0].getAmplitude());
}
