/**
 * @file Grain.cpp
 *
 * This file is part of the Grain class.
*/


#include "Grain.h"
#include <math.h>

Grain::Grain() {
  wave = AudioLab.staticWave(0, SINE);

  attack.curveStep = 1.0;

  release.curveStep = 1.0f;

  sustainAttackAmplitudeDifference = 0;
  sustainAttackFrequencyDifference = 0;
  releaseSustainAmplitudeDifference = 0;
  releaseSustainFrequencyDifference = 0;

  windowCounter = 0;

  state = READY;
}

Grain::Grain(uint8_t aChannel, WaveType aWaveType) {
  wave = AudioLab.staticWave(aChannel, aWaveType);

  attack.curveStep = 1.0;

  release.curveStep = 1.0f;

  sustainAttackAmplitudeDifference = 0;
  sustainAttackFrequencyDifference = 0;
  releaseSustainAmplitudeDifference = 0;
  releaseSustainFrequencyDifference = 0;

  windowCounter = 0;

  state = READY;
}

void Grain::setAttack(float aFrequency, float anAmplitude, int aDuration) {
  attack.frequency = aFrequency;
  attack.amplitude = anAmplitude;
  attack.duration = aDuration;

  sustainAttackFrequencyDifference = sustain.frequency - attack.frequency;
  sustainAttackAmplitudeDifference = sustain.amplitude - attack.amplitude;

  if (attack.duration > 0) {
    attack.curveStep = 1.0 / attack.duration;
  } else {
    attack.curveStep = 1.0;
  }
}

void Grain::setAttackCurve(float aCurveValue) {
  attack.curve = aCurveValue;
}

void Grain::setSustain(float aFrequency, float anAmplitude, int aDuration) {
  sustain.frequency = aFrequency;
  sustain.amplitude = anAmplitude;
  sustain.duration = aDuration;

  sustainAttackFrequencyDifference = sustain.frequency - attack.frequency;
  sustainAttackAmplitudeDifference = sustain.amplitude - attack.amplitude;

  releaseSustainFrequencyDifference = release.frequency - sustain.frequency;
  releaseSustainAmplitudeDifference = release.amplitude - sustain.amplitude;
}

void Grain::setRelease(float aFrequency, float anAmplitude, int aDuration) {
  release.frequency = aFrequency;
  release.amplitude = anAmplitude;
  release.duration = aDuration;

  releaseSustainFrequencyDifference = release.frequency - sustain.frequency;
  releaseSustainAmplitudeDifference = release.amplitude - sustain.amplitude;

  if (release.duration > 0) {
    release.curveStep = 1.0 / release.duration;
  } else {
    release.curveStep = 1.0;
  }
}

void Grain::setReleaseCurve(float aCurveValue) {
  release.curve = aCurveValue;
}

void Grain::setChannel(uint8_t aChannel) {
  wave->setChannel(aChannel);
}

void Grain::setWaveType(WaveType aWaveType) {
  AudioLab.changeWaveType(wave, aWaveType);
}

// TODO: Further improvements to this mess of a switch statement
void Grain::run() {
  switch (state)
  {
    case READY:
      windowCounter = 0;
      state = ATTACK;
      return;

    case ATTACK:
      if (windowCounter < attack.duration) {
        float _curvePosition = pow(attack.curveStep * windowCounter, attack.curve);
        grainFrequency = attack.frequency + sustainAttackFrequencyDifference * _curvePosition;
        grainAmplitude = attack.amplitude + sustainAttackAmplitudeDifference * _curvePosition;
      } else {
        windowCounter = 0;
        state = SUSTAIN;
        grainFrequency = sustain.frequency;
        grainAmplitude = sustain.amplitude;
      }
      break;

    case SUSTAIN:
      if (windowCounter >= sustain.duration) {
        windowCounter = 0;
        state = RELEASE;
      }
      grainFrequency = sustain.frequency;
      grainAmplitude = sustain.amplitude;
      break;

    case RELEASE:
      if (windowCounter < release.duration) {
        float _curvePosition = pow(release.curveStep * windowCounter, release.curve);
        grainFrequency = sustain.frequency + releaseSustainFrequencyDifference * _curvePosition;
        grainAmplitude = sustain.amplitude + releaseSustainAmplitudeDifference * _curvePosition;
      } else {
        wave->reset();
        windowCounter = 0;
        state = READY;
        grainFrequency = 0;
        grainAmplitude = 0;
        return;
      }
      break;

    default:
      Serial.printf("Error: Invalid Grain State");
      break;
  }

  wave->setFrequency(grainFrequency);
  wave->setAmplitude(grainAmplitude);

  windowCounter += 1;
}

grainState Grain::getGrainState() {
  return state;
}

void Grain::update(GrainList *globalGrainList) {
  GrainNode *current = globalGrainList->getHead();
  while (current != NULL) {
    current->reference->run();
    current = current->next;
  }
}

float Grain::getAmplitude() {
  return grainAmplitude;
}

float Grain::getFrequency() {
  return grainFrequency;
}

void GrainList::pushGrain(Grain* grain) {
  GrainNode *newGrain = new GrainNode(grain);
  if (!head) {
    head = tail = newGrain;
  } else {
    tail->next = newGrain;
    tail = newGrain;
  }
}

void GrainList::clearList() {
  GrainNode *current = head;
  while (current) {
    GrainNode *next = current->next;
    delete current->reference;
    delete current;
    current = next;
  }
  head = tail = nullptr;
}

GrainNode* GrainList::getHead() {
  return head;
}
