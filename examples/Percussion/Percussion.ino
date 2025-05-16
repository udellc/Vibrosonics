/**
 * @file Percussion.ino
 *
 * This example shows how to detect percussion.
 */

#include "VibrosonicsAPI.h"

VibrosonicsAPI vapi = VibrosonicsAPI();

float windowData[WINDOW_SIZE_BY_2];
float filteredData[WINDOW_SIZE_BY_2] = { 0 };
float smoothedData[WINDOW_SIZE_BY_2] = { 0 };

Spectrogram rawSpectrogram = Spectrogram(2);
Spectrogram percussionSpectrogram = Spectrogram(2);

ModuleGroup modules = ModuleGroup(&rawSpectrogram);

PercussionDetection percussionDetection = PercussionDetection(0.5, 1800000, 0.75);

FreqEnv dynamicFreqEnv = {};
AmpEnv dynamicAmpEnv = {};

int count = 0;

void setup() {
  Serial.begin(115200);
  vapi.init();

  percussionDetection.setDebugMode(0x01);
  modules.addModule(&percussionDetection, 1800, 4000);

  dynamicFreqEnv = vapi.createFreqEnv(110, 110);
  dynamicAmpEnv = vapi.createAmpEnv(0.5, 0.0, 1, 0, 1, 3, 1.0);
}

void loop() {
  if (!AudioLab.ready()) {
    return;
  }

  vapi.processAudioInput(windowData);

  // floor most noise
  vapi.noiseFloor(windowData, WINDOW_SIZE_BY_2, 300);

  // calculate the entropy of the raw audio data
  float entropy = AudioPrism::entropy(windowData, 0, WINDOW_SIZE_BY_2);

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

  rawSpectrogram.pushWindow(windowData);

  modules.runAnalysis();

  Serial.printf("raw entropy: %f\n", entropy);

  if (percussionDetection.getOutput()) {
    Serial.printf("Percussion detected\n");
    vapi.createDynamicGrain(1, SINE, dynamicFreqEnv, dynamicAmpEnv);
  } else {
    Serial.printf("---\n");
  }

  vapi.updateGrains();

  AudioLab.synthesize();
}
