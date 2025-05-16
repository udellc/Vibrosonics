/**
 * @file Percussion.ino
 *
 * This example shows how to detect percussion.
 */

#include "VibrosonicsAPI.h"

VibrosonicsAPI vapi = VibrosonicsAPI();

FreqEnv dynamicFreqEnv = {};
AmpEnv dynamicAmpEnv = {};


int count = 0;

void setup() {
  Serial.begin(115200);
  vapi.init();

  dynamicFreqEnv = vapi.createFreqEnv(110, 110);
  dynamicAmpEnv = vapi.createAmpEnv(0.5, 0.0, 3, 2, 1, 3, 1.0);
}

void loop() {
  if (!AudioLab.ready()) {
    return;
  }

  if (count % 4 == 0) {
    vapi.createDynamicGrain(0, SINE, dynamicFreqEnv, dynamicAmpEnv);
  }


  count++;

  vapi.updateGrains();

  AudioLab.synthesize();
}
