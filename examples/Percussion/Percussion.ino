/**
 * @file Percussion.ino
 *
 * This example showcases how to detect percussion and synthesize the detected
 * hits into haptic feedback. The example features our frequency domain data
 * processing technique to capture the percussive/transient elements of an
 * audio signal. This filtered data is used as the input to an AudioPrism
 * PercussionDetection module, which tells us when to output grains
 * corresponding to snare/hi-hat haptic feedback.
 */

#define PERC_FREQ_LO 1800
#define PERC_FREQ_HI 4000
#define PERC_WAVE_TYPE TRIANGLE

#include "VibrosonicsAPI.h"

VibrosonicsAPI vapi = VibrosonicsAPI();

float windowData[WINDOW_SIZE_BY_2];
float filteredData[WINDOW_SIZE_BY_2] = { 0 };
float smoothedData[WINDOW_SIZE_BY_2] = { 0 };

// Create the spectrogram which will store our filtered percussive data. It
// needs at least two windows to be able to calculate the energy flux between
// windows.
Spectrogram percussiveSpectrogram = Spectrogram(2);

// Create a module group which will use the percussive spectrogram as its
// input.
ModuleGroup modules = ModuleGroup(&percussiveSpectrogram);

// Create the PercussionDetection module. These parameters have been configured
// for the (as of 2024-25) newest version of the Vibrosonics hardware.
PercussionDetection percussionDetection = PercussionDetection(0.5, 100000000, 0.78);

FreqEnv freqEnv = {};
AmpEnv ampEnv = {};
DurEnv durEnv = {};

void setup() {
  Serial.begin(115200);
  vapi.init();

  durEnv = vapi.createDurEnv(1, 0, 1, 3, 1.0);
  // Optional: set the debug mode on the PercussionDetection module:
  // percussionDetection.setDebugMode(0x01);
  
  // Add the PercussionDetection module to our module group.
  modules.addModule(&percussionDetection, PERC_FREQ_LO, PERC_FREQ_HI);
}

void loop() {
  if (!AudioLab.ready()) {
    return;
  }

  // Collect the audio signal data of the recorded window.
  vapi.processAudioInput(windowData);

  // Floor noise from the wire.
  vapi.noiseFloor(windowData, 300);

  // Create a copy of the raw window data for further noise filtering.
  memcpy(filteredData, windowData, WINDOW_SIZE_BY_2 * sizeof(float));

  // Apply CFAR to clean up the data.
  vapi.noiseFloorCFAR(filteredData, 6, 1, 1.4);

  // Smooth the filterd data over a long period of time to capture melodic
  // elements (frequencies present over multiple windows).
  AudioPrism::smooth_window_over_time(filteredData, smoothedData, 0.2);

  // Subtract the melodic data from the raw data to capture the 'percussive
  // data'.
  for (int i = 0; i < WINDOW_SIZE_BY_2; i++) {
    windowData[i] = max(0., windowData[i] - smoothedData[i]);
  }

  // Finally, the window data has been filtered for percussion, so push this
  // into the spectrogram that the PercussionDetection module will use as
  // input.
  percussiveSpectrogram.pushWindow(windowData);

  // Run the PercussionDetection module's analysis function.
  modules.runAnalysis();

  // The output is a boolean indicating if a percussive hit was detected, so
  // output feedback if this is true.
  if (percussionDetection.getOutput()) {
    // Get the energy, entropy and positive flux for the percussive hit. These
    // values are used to synthesize the haptic feedback of the percussion.
    float energy = AudioPrism::energy(windowData, PERC_FREQ_LO, PERC_FREQ_HI);
    float entropy = AudioPrism::entropy(windowData, PERC_FREQ_LO, PERC_FREQ_HI);
    float flux = AudioPrism::positive_flux(windowData,
                                           percussiveSpectrogram.getPreviousWindow(),
                                           PERC_FREQ_LO, PERC_FREQ_HI);

    // Normalize the flux [0.0, 1.0] by the total energy
    flux /= energy;

    // For debugging purposes, output the parameters that caused a percussive
    // hit to be detected
    Serial.printf("Percussion detected\n");
    Serial.printf("- energy: %05g\n", energy);
    Serial.printf("- entropy: %05g\n", entropy);
    Serial.printf("- flux: %05g\n", flux);
  
    // Create the frequency and amplitude envelopes for the percussive hit,
    // using a set frequency of 160 and the energy of the detected hit as the
    // amplitude.
    freqEnv = vapi.createFreqEnv(160, 160, 160, 20);
    ampEnv = vapi.createAmpEnv(energy, energy, 0.3 * energy, 0.);

    synthesizeHit(flux);

    // For particularily noisy hits, synthesize another hit with less energy
    // to create a rougher feeling.
    if (entropy > 0.9) {
      energy *= 0.3;
      freqEnv = vapi.createFreqEnv(200, 200, 200, 20);
      ampEnv = vapi.createAmpEnv(energy, energy, 0.3 * energy, 0.);
      synthesizeHit(flux);
    }
  } else {
    // For debugging purposes, to complement the previous print statements.
    Serial.printf("---\n\n");
  }

  // Update the percussive grains created.
  vapi.updateGrains();

  // Map the amplitudes of both channels for output through the DAC.
  AudioLab.mapAmplitudes(0, 10000000);
  AudioLab.mapAmplitudes(1, 10000000);

  // Synthesize the waves created for haptic feedback.
  AudioLab.synthesize();
}

// Synthesize the percussive hit to either one or both speakers, based on the
// flux. This creates a nice variation between hits with more or less sudden
// energy.
void synthesizeHit(float flux) {
  if (flux < 0.75) {
    vapi.createDynamicGrain(0, PERC_WAVE_TYPE, freqEnv, ampEnv, durEnv);
    vapi.createDynamicGrain(1, PERC_WAVE_TYPE, freqEnv, ampEnv, durEnv);
    Serial.printf("--- channel: 0\n");
  } else {
    vapi.createDynamicGrain(1, PERC_WAVE_TYPE, freqEnv, ampEnv, durEnv);
    Serial.printf("--- channel: 1 & 0\n");
  }
}
