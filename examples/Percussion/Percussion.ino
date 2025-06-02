/**
 * @file Percussion.ino
 *
 * This example shows how to detect percussion.
 */

#define PERC_FREQ_LO 1800
#define PERC_FREQ_HI 4000

#include "VibrosonicsAPI.h"

VibrosonicsAPI vapi = VibrosonicsAPI();

float windowData[WINDOW_SIZE_BY_2];
float filteredData[WINDOW_SIZE_BY_2] = { 0 };
float smoothedData[WINDOW_SIZE_BY_2] = { 0 };

Spectrogram percussiveSpectrogram = Spectrogram(2);

ModuleGroup modules = ModuleGroup(&percussiveSpectrogram);

PercussionDetection percussionDetection = PercussionDetection(0.5, 100000000, 0.78);

FreqEnv freqEnv = {};
AmpEnv ampEnv = {};

void setup() {
  Serial.begin(115200);
  vapi.init();

  // percussionDetection.setDebugMode(0x01);
  modules.addModule(&percussionDetection, PERC_FREQ_LO, PERC_FREQ_HI);

  freqEnv = vapi.createFreqEnv(110, 110, 110, 20);
  ampEnv = vapi.createAmpEnv(0.5, 1, 0.5, 2, 0.4, 1, 0.0, 3, 1.0);
}

void loop() {
  if (!AudioLab.ready()) {
    return;
  }

  vapi.processAudioInput(windowData);

  // floor most noise
  vapi.noiseFloor(windowData, WINDOW_SIZE_BY_2, 300);

  // calculate the entropy of the raw audio data

  // copy the windowData to filterdData
  memcpy(filteredData, windowData, WINDOW_SIZE_BY_2);

  // apply CFAR to clean up the data
  vapi.noiseFloorCFAR(filteredData, WINDOW_SIZE_BY_2, 6, 1, 1.4);

  // smooth over a long period of time to capture melodic elements
  AudioPrism::smooth_window_over_time(filteredData, smoothedData, 0.2);

  // subtract the melodic data from the raw data to capture the 'percussive data'
  for (int i = 0; i < WINDOW_SIZE_BY_2; i++) {
    windowData[i] = windowData[i] - smoothedData[i];
    if (windowData[i] < 0) {
      windowData[i] = 0;
    }
  }

  percussiveSpectrogram.pushWindow(windowData);

  modules.runAnalysis();

  //Serial.printf("raw entropy: %f\n", entropy);

  if (percussionDetection.getOutput()) {
    float energy = AudioPrism::energy(windowData, PERC_FREQ_LO, PERC_FREQ_HI);
    float flux = AudioPrism::positive_flux(windowData,
                                           percussiveSpectrogram.getPreviousWindow(),
                                           PERC_FREQ_LO, PERC_FREQ_HI);
    flux /= energy;
    float entropy = AudioPrism::entropy(windowData, PERC_FREQ_LO, PERC_FREQ_HI);

    Serial.printf("Percussion detected\n");
    Serial.printf("- flux: %05g\n", flux);
    Serial.printf("- energy: %05g\n", energy);
    Serial.printf("- entropy: %05g\n", entropy);
  
    freqEnv = vapi.createFreqEnv(160, 160, 160, 20);
    ampEnv = vapi.createAmpEnv(energy, 1, energy, 0, 0.3 * energy, 1, 0., 3, 1);

    synthesizeHit(flux);

    if (entropy > 0.9) {
      energy *= 0.3;
      freqEnv = vapi.createFreqEnv(160, 160, 160, 20);
      ampEnv = vapi.createAmpEnv(energy, 1, energy, 0, 0.3 * energy, 1, 0., 3, 1);
      synthesizeHit(flux);
    }
  } else {
    Serial.printf("---\n\n");
  }

  vapi.updateGrains();

  AudioLab.mapAmplitudes(0, 10000000);
  AudioLab.mapAmplitudes(1, 10000000);

  AudioLab.synthesize();
}

void synthesizeHit(float flux) {
  if (flux < 0.75) {
    vapi.createDynamicGrain(0, TRIANGLE, freqEnv, ampEnv);
    vapi.createDynamicGrain(1, TRIANGLE, freqEnv, ampEnv);
    Serial.printf("--- channel: 0\n");
  } else {
    vapi.createDynamicGrain(1, TRIANGLE, freqEnv, ampEnv);
    Serial.printf("--- channel: 1 & 0\n");
  }
}
