/**
 * @file Grain.cpp
 *
 * This file is part of the Grain class.
*/


#include "Grain.h"
#include <math.h>

Grain::GrainNode *Grain::globalGrainList = NULL;

void Grain::pushGrainNode(void) {
  if (globalGrainList == NULL) {
    globalGrainList = new GrainNode(this);
    return;
  }

  GrainNode *_currentNode = globalGrainList;
  // NOTE: This seems really slow, not sure why they did this
  while (_currentNode->next != NULL) {
    _currentNode = _currentNode->next;
  }

  _currentNode->next = new GrainNode(this);
}

Grain::Grain() {
  pushGrainNode();
  wave = AudioLab.staticWave(0, SINE);

  attackDuration = 0;
  attackFrequency = 0;
  attackAmplitude = 0;
  attackCurve = 1.0;
  attackCurveStep = 1.0;

  sustainDuration = 0;
  sustainFrequency = 0;
  sustainAmplitude = 0;

  releaseDuration = 0;
  releaseFrequency = 0;
  releaseAmplitude = 0;
  releaseCurve = 1.0;
  releaseCurveStep = 1.0;

  sustainAttackAmplitudeDifference = 0;
  sustainAttackFrequencyDifference = 0;
  releaseSustainAmplitudeDifference = 0;
  releaseSustainFrequencyDifference = 0;

  windowCounter = 0;

  state = READY;
}

Grain::Grain(uint8_t aChannel, WaveType aWaveType) {
  pushGrainNode();
  wave = AudioLab.staticWave(aChannel, aWaveType);

  attackDuration = 0;
  attackFrequency = 0;
  attackAmplitude = 0;
  attackCurve = 1.0;
  attackCurveStep = 1.0;

  sustainDuration = 0;
  sustainFrequency = 0;
  sustainAmplitude = 0;

  releaseDuration = 0;
  releaseFrequency = 0;
  releaseAmplitude = 0;
  releaseCurve = 1.0;
  releaseCurveStep = 1.0;

  sustainAttackAmplitudeDifference = 0;
  sustainAttackFrequencyDifference = 0;
  releaseSustainAmplitudeDifference = 0;
  releaseSustainFrequencyDifference = 0;

  windowCounter = 0;

  state = READY;
}

void Grain::start(void) {
  state = ATTACK;
  windowCounter = 0;
}

void Grain::stop(void) {
  state = READY;
  windowCounter = 0;
  grainAmplitude = 0;
  grainFrequency = 0;
}

void Grain::setAttack(float aFrequency, float anAmplitude, int aDuration) {
  attackFrequency = aFrequency;
  attackAmplitude = anAmplitude;
  attackDuration = aDuration;

  sustainAttackFrequencyDifference = sustainFrequency - attackFrequency;
  sustainAttackAmplitudeDifference = sustainAmplitude - attackAmplitude;

  if (attackDuration > 0) {
    attackCurveStep = 1.0 / attackDuration;
  } else {
    attackCurveStep = 1.0;
  }
}

void Grain::setAttackCurve(float aCurveValue) {
  attackCurve = aCurveValue;
}

void Grain::setSustain(float aFrequency, float anAmplitude, int aDuration) {
  sustainFrequency = aFrequency;
  sustainAmplitude = anAmplitude;
  sustainDuration = aDuration;

  sustainAttackFrequencyDifference = sustainFrequency - attackFrequency;
  sustainAttackAmplitudeDifference = sustainAmplitude - attackAmplitude;

  releaseSustainFrequencyDifference = releaseFrequency - sustainFrequency;
  releaseSustainAmplitudeDifference = releaseAmplitude - sustainAmplitude;
}

void Grain::setRelease(float aFrequency, float anAmplitude, int aDuration) {
  releaseFrequency = aFrequency;
  releaseAmplitude = anAmplitude;
  releaseDuration = aDuration;

  releaseSustainFrequencyDifference = releaseFrequency - sustainFrequency;
  releaseSustainAmplitudeDifference = releaseAmplitude - sustainAmplitude;

  if (releaseDuration > 0) {
    releaseCurveStep = 1.0 / releaseDuration;
  } else {
    releaseCurveStep = 1.0;
  }
}

void Grain::setReleaseCurve(float aCurveValue) {
  releaseCurve = aCurveValue;
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
      return;

    case ATTACK:
      if (windowCounter < attackDuration) {
        float _curvePosition = pow(attackCurveStep * windowCounter, attackCurve);
        grainFrequency = attackFrequency + sustainAttackFrequencyDifference * _curvePosition;
        grainAmplitude = attackAmplitude + sustainAttackAmplitudeDifference * _curvePosition;
      } else {
        windowCounter = 0;
        state = SUSTAIN;
        grainFrequency = sustainFrequency;
        grainAmplitude = sustainAmplitude;
      }
      break;

    case SUSTAIN:
      if (windowCounter >= sustainDuration) {
        windowCounter = 0;
        state = RELEASE;
      }
      grainFrequency = sustainFrequency;
      grainAmplitude = sustainAmplitude;
      break;

    case RELEASE:
      if (windowCounter < releaseDuration) {
        float _curvePosition = pow(releaseCurveStep * windowCounter, releaseCurve);
        grainFrequency = sustainFrequency + releaseSustainFrequencyDifference * _curvePosition;
        grainAmplitude = sustainAmplitude + releaseSustainAmplitudeDifference * _curvePosition;
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

void Grain::update() {
  GrainNode* _currentNode = globalGrainList;

  while (_currentNode != NULL) {
    _currentNode->reference->run();
    _currentNode = _currentNode->next;
  }
}

float Grain::getAmplitude() {
  return grainAmplitude;
}

float Grain::getFrequency() {
  return grainFrequency;
}
