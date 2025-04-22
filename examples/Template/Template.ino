/**
 * @file template.ino
 *
 * This example serves a reference point for creating your own Vibrosonics
 * programs.
 */

#include "VibrosonicsAPI.h"

// Set the number of peaks for the major peaks module to find
// Currently set to the default amount of 4
#define NUM_PEAKS 4

/**
 * Instance of the Vibrosonics API. Core to all operations including:
 * -- FFT operations with Fast4ier
 * -- Audio spectrum storage with the spectrogram
 * -- Audio input and synthesis through AudioLab
 * -- AudioPrism module management and analysis
 */
VibrosonicsAPI vapi = VibrosonicsAPI();

// DATA
// Array that stores the AudioLab input buffer time domain data
// After FFT processing it becomes the current window's spectrogram data
float windowData[WINDOW_SIZE_BY_2];
// Stores 2 windows of raw unprocessed data
// You may want to use raw spectrogram data if you are using the
// noisiness module
Spectrogram rawSpectrogram = Spectrogram(2);
// Stores 2 windows of processed data
// Most of the time you will be using processed data for analysis
Spectrogram processedSpectrogram = Spectrogram(2);

// MODULES
// Create the ModuleGroup to store initialized modules
ModuleGroup modules = ModuleGroup(&processedSpectrogram);
// Creates major peaks module to analyze defined amount of peaks
MajorPeaks majorPeaks = MajorPeaks(NUM_PEAKS);

/**
 * Runs once on ESP32 startup.
 * VibrosonicsAPI initializes AudioLab
 * User defined modules are added to the ModuleGroup
 * Grain shaping needs to be configured here.
 */
void setup()
{
  Serial.begin(115200);
  vapi.init();
  // Add any AudioPrism modules to the ModuleGroup here
  // Here, majorPeaks is added with a frequency analysis range of
  // 20hz-3000hz
  modules.addModule(&majorPeaks, 20, 3000);
}

/**
 * Main loop for analysis and synthesis.
 * Has 5 key steps:
 * 1. Check if AudioLab is ready
 * 2. Process input
 * 3. Analyze processed data with modules
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
  // Vibrosonics API performs these steps:
  // Copies samples from the AudioLab buffer to the vReal array
  // Perfroms FFT operations on vReal
  vapi.processAudioInput(windowData);
  // Push the FFT data to the raw spectrogram
  rawSpectrogram.pushWindow(windowData);

  // Using this noise flooring function helps with getting a clear
  // sounding output
  vapi.noiseFloorCFAR(windowData, WINDOW_SIZE_BY_2, 4, 1, 1.6);
  // Push the processed data to the processed spectrogram
  processedSpectrogram.pushWindow(windowData);

  // Third, analyze the data with the added AudioPrism modules
  modules.runAnalysis();

  // Fourth, generate waves
  // Get the analyzed data from MajorPeaks module
  float** peaksData = majorPeaks->getOutput();
  // Map the amplitudes for a clearer output
  vapi.mapAmplitudes(peaksData[MP_AMP], NUM_PEAKS, 10000);
  // Assign frequency and amplitude data to be outputted on both output channels.
  vapi.assignWaves(peaksData[MP_FREQ], peaksData[MP_AMP], NUM_PEAKS, 0);
  vapi.assignWaves(peaksData[MP_FREQ], peaksData[MP_AMP], NUM_PEAKS, 1);

  // Perform any additional manipulation that you want here

  // Lastly, synthesize all created waves through AudioLab
  AudioLab.synthesize();
  // Uncomment for debugging:
  // AudioLab.printWaves();
}
