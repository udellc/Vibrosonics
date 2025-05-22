/**
 * @file Grains.ino
 *
 * This example serves a reference point for using granular synthesis
 * through grains.
 * It demonstrates a frequency and amplitude sweep, duration changes,
 * wave shape variations, and how each parameter functions in isolation.
 */

#include "VibrosonicsAPI.h"

// Grain Parameters
#define ATTACK_DURATION 10
#define DECAY_DURATION 3
#define SUSTAIN_DURATION 2
#define RELEASE_DURATION 8
#define CURVE 1.0
#define MAX_AMP 0.5
#define MIN_AMP 0.0

#define MAX_FREQ 1000
#define START_FREQ 20
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
FreqEnv sweepFreqEnv = {};
AmpEnv sweepAmpEnv = {};

float targetFreq = START_FREQ;
/**
 * Runs once on ESP32 startup.
 * VibrosonicsAPI initializes AudioLab and adds modules to be managed
 * Grain shaping should also be configured here.
 */
void setup()
{
  Serial.begin(115200);
  vapi.init();
  sweepFreqEnv = vapi.createFreqEnv(START_FREQ, START_FREQ, START_FREQ, 0.0);
  sweepAmpEnv = vapi.createAmpEnv(MAX_AMP, ATTACK_DURATION, MAX_AMP, DECAY_DURATION, MAX_AMP, SUSTAIN_DURATION, MIN_AMP, RELEASE_DURATION, CURVE);
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
  if(sweepGrain[0].getFrequency() >= MAX_FREQ) targetFreq = START_FREQ;
  // Previous frequency has finished playing, increase grain frequency
  if(sweepGrain[0].getGrainState() == READY){
    targetFreq = targetFreq*1.0595;
    sweepFreqEnv.targetFrequency = targetFreq;
    vapi.triggerGrains(sweepGrain, 1, sweepFreqEnv, sweepAmpEnv);
  }
  // Progress the grain through its curve
  vapi.updateGrains();
  // Use AudioLab to synthesize waves for output
  AudioLab.synthesize();
  // Uncomment for debugging:
  sweepGrain->printGrain();
}
