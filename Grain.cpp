/**
 * @file Grain.cpp
 *
 * This file is part of the Grain class.
*/


#include "Grain.h"
#include <math.h>

/**
 * Creates a grain on channel 0 and sine wave type in
 * the ready state.
 */
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

/**
 * Creates a grain on the specified channel and with the
 * inputted wave type in the ready state.
 *
 * @param aChannel Specified channel for output
 * @param aWaveType Specified wave type for output
 */
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

/**
 * Updates frequency, amplitude, and duration. Also updates sustain
 * frequency and amplitude difference.
 *
 * @param aFrequency Updated frequncy
 * @param anAmplitude Updated amplitude
 * @param aDuration Updated duration
 */
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

/**
 * Updates the attack curve value.
 *
 * @param aCurveValue Updated curve value
 */
void Grain::setAttackCurve(float aCurveValue) {
  attack.curve = aCurveValue;
}

/**
 * Updates frequency, amplitude, and duration. Also updates attack
 * frequency and amplitude difference.
 *
 * @param aFrequency Updated frequncy
 * @param anAmplitude Updated amplitude
 * @param aDuration Updated duration
 */
void Grain::setSustain(float aFrequency, float anAmplitude, int aDuration) {
  sustain.frequency = aFrequency;
  sustain.amplitude = anAmplitude;
  sustain.duration = aDuration;

  sustainAttackFrequencyDifference = sustain.frequency - attack.frequency;
  sustainAttackAmplitudeDifference = sustain.amplitude - attack.amplitude;

  releaseSustainFrequencyDifference = release.frequency - sustain.frequency;
  releaseSustainAmplitudeDifference = release.amplitude - sustain.amplitude;
}

/**
 * Updates frequency, amplitude, and duration. Also updates attack
 * and sustain frequency and amplitude difference.
 *
 * @param aFrequency Updated frequncy
 * @param anAmplitude Updated amplitude
 * @param aDuration Updated duration
 */
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

/**
 * Updates release curve value
 *
 * @param aCurveValue Updated curve value
 */
void Grain::setReleaseCurve(float aCurveValue) {
  release.curve = aCurveValue;
}

/**
 * Sets the channel of this grain
 *
 * @param aChannel The channel on specified integer
 */
void Grain::setChannel(uint8_t aChannel) {
  wave->setChannel(aChannel);
}

/**
 * Sets grain wave type (SINE, COSINE, SQUARE, TRIANGLE, SAWTOOTH)
 *
 * @param aWaveType The wave type
 */
void Grain::setWaveType(WaveType aWaveType) {
  AudioLab.changeWaveType(wave, aWaveType);
}

/**
 * Updates wave frequency and amplitude along with the window counter.
 * Switches grain states based on the window counter and durations for
 * each state. In essence it progresses the sample along the attack sustain
 * release curve.
 */
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


/**
 * Returns the state of a grain (READY, ATTACK, SUSTAIN, RELEASE)
 *
 * @return grainState
 */
grainState Grain::getGrainState() {
  return state;
}

/**
 * Performs the run() function on each node in the globalGrainList
 */
void Grain::update(GrainList *globalGrainList) {
  GrainNode *current = globalGrainList->getHead();
  while (current != NULL) {
    current->reference->run();
    current = current->next;
  }
}

/**
 * Returns the current amplitude of the grain
 *
 * @return float
 */
float Grain::getAmplitude() {
  return grainAmplitude;
}

/**
 * Returns the current frequency of the grain
 *
 * @return float
 */
float Grain::getFrequency() {
  return grainFrequency;
}

/**
 * Pushes a grain to the tail of the GrainList.
 *
 * @param grain the grain to be pushed
 */
void GrainList::pushGrain(Grain* grain) {
  GrainNode *newGrain = new GrainNode(grain);
  if (!head) {
    head = tail = newGrain;
  } else {
    tail->next = newGrain;
    tail = newGrain;
  }
}

/**
 * Deletes all grains in the GrainList.
 */
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

/**
 * Returns the head of the GrainList.
 *
 * @return GrainNode*
 */
GrainNode* GrainList::getHead() {
  return head;
}
