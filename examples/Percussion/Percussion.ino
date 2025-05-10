/**
 * @file Percussion.ino
 *
 * This example shows how to detect percussion.
 */

#include "VibrosonicsAPI.h"

VibrosonicsAPI vapi = VibrosonicsAPI();

float windowData[WINDOW_SIZE_BY_2];

Spectrogram rawSpectrogram = Spectrogram(2);

ModuleGroup modules = ModuleGroup(&rawSpectrogram);

PercussionDetection percussionDetection = PercussionDetection();
Grain* percussionGrain = vapi.createGrainArray(1, 0, SINE);

int count = 0;

void setup()
{
  Serial.begin(115200);
  vapi.init();

  percussionDetection.setDebugMode(0x01);
  modules.addModule(&percussionDetection, 1800, 4000);

  vapi.shapeGrainAttack(percussionGrain, 1, 1, 1.0, 1.0, 1.0);
  vapi.shapeGrainSustain(percussionGrain, 1, 1, 1.0, 1.0);
  vapi.shapeGrainRelease(percussionGrain, 1, 3, 1.0, 1.0, 1.0);
}

void loop()
{
  if (!AudioLab.ready()) {
    return;
  }

  count++;

  vapi.processAudioInput(windowData);
  vapi.noiseFloor(windowData, WINDOW_SIZE_BY_2, 200);

  rawSpectrogram.pushWindow(windowData);

  modules.runAnalysis();

  if (percussionDetection.getOutput()) {
    Serial.printf("Percussion detected\n");

    percussionGrain[0].setAttack(100, 0.5, percussionGrain[0].getAttackDuration());
    percussionGrain[0].setSustain(100, 0.5, percussionGrain[0].getSustainDuration());
    percussionGrain[0].setRelease(100, 0.0, percussionGrain[0].getReleaseDuration());
    percussionGrain[0].transitionTo(ATTACK);
  } else {
    Serial.printf("---\n");

    // percussionGrain[0].setAttack(100, 0.0, percussionGrain[0].getAttackDuration());
    // percussionGrain[0].setSustain(100, 0.0, percussionGrain[0].getSustainDuration());
    // percussionGrain[0].setRelease(100, 0.0, percussionGrain[0].getReleaseDuration());
  }

  vapi.updateGrains();

  AudioLab.synthesize();
}
