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
Grain::Grain()
{
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
 * @param channel Specified channel for output
 * @param waveType Specified wave type for output
 */
Grain::Grain(uint8_t channel, WaveType waveType)
{
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
 * @param frequency Updated frequncy
 * @param amplitude Updated amplitude
 * @param duration Updated duration
 */
void Grain::setAttack(float frequency, float amplitude, int duration)
{
  attack.frequency = frequency;
  attack.amplitude = amplitude;
  attack.duration = duration;

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
 * @param curveValue Updated curve value
 */
void Grain::setAttackCurve(float curveValue)
{
  attack.curve = curveValue;
}

/**
 * Updates frequency, amplitude, and duration. Also updates attack
 * frequency and amplitude difference.
 *
 * @param frequency Updated frequncy
 * @param amplitude Updated amplitude
 * @param duration Updated duration
 */
void Grain::setSustain(float frequency, float amplitude, int duration)
{
  sustain.frequency = frequency;
  sustain.amplitude = amplitude;
  sustain.duration = duration;

  sustainAttackFrequencyDifference = sustain.frequency - attack.frequency;
  sustainAttackAmplitudeDifference = sustain.amplitude - attack.amplitude;

  releaseSustainFrequencyDifference = release.frequency - sustain.frequency;
  releaseSustainAmplitudeDifference = release.amplitude - sustain.amplitude;
}

/**
 * Updates frequency, amplitude, and duration. Also updates attack
 * and sustain frequency and amplitude difference.
 *
 * @param frequency Updated frequncy
 * @param amplitude Updated amplitude
 * @param duration Updated duration
 */
void Grain::setRelease(float frequency, float amplitude, int duration)
{
  release.frequency = frequency;
  release.amplitude = amplitude;
  release.duration = duration;

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
 * @param curveValue Updated curve value
 */
void Grain::setReleaseCurve(float curveValue)
{
  release.curve = curveValue;
}

/**
 * Sets the channel of this grain
 *
 * @param channel The channel on specified integer
 */
void Grain::setChannel(uint8_t channel)
{
  grainChannel = channel;
}

/**
 * Sets grain wave type (SINE, COSINE, SQUARE, TRIANGLE, SAWTOOTH)
 *
 * @param waveType The wave type
 */
void Grain::setWaveType(WaveType waveType)
{
  this->waveType = waveType;
}

/**
 * Updates wave frequency and amplitude along with the window counter.
 * Switches grain states based on the window counter and durations for
 * each state. In essence it progresses the sample along the attack sustain
 * release curve.
 */
void Grain::run()
{
  switch (state)
  {
    case READY:
      windowCounter = 0;
      state = ATTACK;
      return;

    case ATTACK:
      if (windowCounter < attack.duration) {
        float curvePosition = pow(attack.curveStep * windowCounter, attack.curve);
        grainFrequency = attack.frequency + sustainAttackFrequencyDifference * curvePosition;
        grainAmplitude = attack.amplitude + sustainAttackAmplitudeDifference * curvePosition;
      } else {
        windowCounter = 0;
        state = SUSTAIN;
        grainFrequency = sustain.frequency;
        grainAmplitude = sustain.amplitude;
      }
      break;

    case SUSTAIN:
      if (windowCounter < sustain.duration) {
        grainFrequency = sustain.frequency;
        grainAmplitude = sustain.amplitude;
      } else {
        windowCounter = 0;
        state = RELEASE;
      }
      break;

    case RELEASE:
      if (windowCounter < release.duration) {
        float curvePosition = pow(release.curveStep * windowCounter, release.curve);
        grainFrequency = sustain.frequency + releaseSustainFrequencyDifference * curvePosition;
        grainAmplitude = sustain.amplitude + releaseSustainAmplitudeDifference * curvePosition;
      } else {
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

  // Create a wave
  Wave wave = AudioLab.dynamicWave(grainChannel, grainFrequency, grainAmplitude);
  windowCounter += 1;
}


/**
 * Returns the state of a grain (READY, ATTACK, SUSTAIN, RELEASE)
 *
 * @return grainState
 */
grainState Grain::getGrainState()
{
  return state;
}

/**
 * Performs the run() function on each node in the globalGrainList
 */
void Grain::update(GrainList *globalGrainList)
{
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
float Grain::getAmplitude()
{
  return grainAmplitude;
}

/**
 * Returns the current frequency of the grain
 *
 * @return float
 */
float Grain::getFrequency()
{
  return grainFrequency;
}

/**
 * Returns the attack window duration
 *
 * @return int
 */
int Grain::getAttackDuration()
{
  return attack.duration;
}

/**
 * Returns the sustain window duration
 *
 * @return int
 */
int Grain::getSustainDuration()
{
  return sustain.duration;
}

/**
 * Returns the release window duration
 *
 * @return int
 */
int Grain::getReleaseDuration()
{
  return release.duration;
}

/**
 * Pushes a grain to the tail of the GrainList.
 *
 * @param grain the grain to be pushed
 */
void GrainList::pushGrain(Grain* grain)
{
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
void GrainList::clearList()
{
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
GrainNode* GrainList::getHead()
{
  return head;
}
