/**
 * @file major_peaks.ino
 *
 * This example serves a reference point for using the MajorPeaks module from AudioPrism.
 * MajorPeaks analyzes the current window's spectrogram to identify the frequency bin/bins with the greatest amplitude/amplitudes.
 */
#include "VibrosonicsAPI.h"

// Number of peaks module will look for
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

// Stores 2 windows of processed data
// Most of the time you will be using processed data for analysis
Spectrogram processedSpectrogram = Spectrogram(2, WINDOW_SIZE_OVERLAP);

// MODULES
// Create the ModuleGroup to store initialized modules
ModuleGroup modules = ModuleGroup(&processedSpectrogram);

/**
 * Here we create the MajorPeaks module we will use in this example
 * NUM_PEAKS sets the amount of peaks the module will search for
 * By default if no arguement is provided it will search for 4 peaks
 */
MajorPeaks majorPeaks = MajorPeaks(NUM_PEAKS);

/**
 * Runs once on ESP32 startup.
 * Baud rate is configured for ESP32
 */
void setup()
{
  Serial.begin(115200);
  vapi.init();
  // Here, majorPeaks is added with a frequency analysis range of
  // 20hz-3000hz
  majorPeaks.setWindowSize(WINDOW_SIZE_OVERLAP);
  modules.addModule(&majorPeaks, 20, 3000);
}

void loop()
{
  // Check to make sure that the AudioLab input buffer has been filled
  if (!vapi.isAudioLabReady()) {
    return;
  }

  // Process the input data
  vapi.processAudioInput(windowData);

  // Using this noise flooring function helps with getting a clear
  // sounding output. This is more useful on the original prototype.
  // You may not need this if you are using the latest hardware.
  vapi.noiseFloorCFAR(windowData, 4, 1, 1.6);

  // Push the processed data to the processed spectrogram
  processedSpectrogram.pushWindow(windowData);

  // Analyze the data with the added AudioPrism modules
  modules.runAnalysis();

  // Get the analyzed data from MajorPeaks module
  float** peaksData = majorPeaks.getOutput();

  /**
   * Now that we have the data from MajorPeaks' analysis, we can
   * decide what to do with that data.
   * Some examples of what to do with the data are:
   * -- Output the wave peaks
   * -- Print out the info about the found peaks
   * Both of these examples are shown below
   */

  // Print out peak data
  Serial.printf("Major Peaks:\n");
  for (int i = 0; i < NUM_PEAKS; i++){
    Serial.printf("Peak: %i Frequency: %fHz Amplitude: %f\n", i, FREQ_RES * peaksData[MP_FREQ][i], peaksData[MP_AMP][i]);
  }

  // Generate waves to be outputted on the hardware on channel 0
  vapi.assignWaves(peaksData[MP_FREQ], peaksData[MP_AMP], NUM_PEAKS, 0);

  // Synthesize all created waves through AudioLab
  AudioLab.synthesize();
  // Uncomment for debugging outputted waves:
  // AudioLab.printWaves();
}
