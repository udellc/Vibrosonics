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
  grainChannel = 0;
  waveType = SINE;
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
  grainChannel = channel;
  this->waveType = waveType;
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
  attack.frequency = frequency * attack.frequencyModulator;
  attack.amplitude = amplitude * attack.amplitudeModulator;
  attack.duration = duration;

  sustainAttackFrequencyDifference = sustain.frequency - attack.frequency;
  sustainAttackAmplitudeDifference = sustain.amplitude - attack.amplitude;

  attack.curveStep = (duration > 0) ? 1.0 / duration : 1.0;
}

/**
 * Updates the attack curve value.
 *
 * @param curveValue Updated curve value
 */
void Grain::setAttackCurve(float curveValue)
{
  attack.curve = curveValue;
  attack.incrementFactor = pow(1.0 - attack.curveStep, attack.curve);
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
  sustain.frequency = frequency * sustain.frequencyModulator;
  sustain.amplitude = amplitude * sustain.amplitudeModulator;
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
  release.frequency = frequency * release.frequencyModulator;
  release.amplitude = amplitude * release.amplitudeModulator;
  release.duration = duration;

  releaseSustainFrequencyDifference = release.frequency - sustain.frequency;
  releaseSustainAmplitudeDifference = release.amplitude - sustain.amplitude;

  release.curveStep = (duration > 0) ? 1.0 / duration : 1.0;
}

/**
 * Updates release curve value
 *
 * @param curveValue Updated curve value
 */
void Grain::setReleaseCurve(float curveValue)
{
  release.curve = curveValue;
  release.incrementFactor = pow(1.0 - release.curveStep, release.curve);
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
      transitionTo(ATTACK);
      break;

    case ATTACK:
      if (windowCounter < attack.duration) {
        // Compute and update frequncy and amplitude with incremental curve position
        grainFrequency = attack.frequency + sustainAttackFrequencyDifference * attack.curvePosition;
        grainAmplitude = attack.amplitude + sustainAttackAmplitudeDifference * attack.curvePosition;

        // Update curve position incrementally
        // nextCurvePosition = previousCurvePosition * incrementFactor
        attack.curvePosition *= attack.incrementFactor;
      } else {
        transitionTo(SUSTAIN);
      }
      break;

    case SUSTAIN:
      if (windowCounter >= sustain.duration) {
        transitionTo(RELEASE);
      }
      break;

    case RELEASE:
      if (windowCounter < release.duration) {
        // Compute and update frequncy and amplitude with incremental curve position
        grainFrequency = sustain.frequency + releaseSustainFrequencyDifference * release.curvePosition;
        grainAmplitude = sustain.amplitude + releaseSustainAmplitudeDifference * release.curvePosition;

        // Update curve position incrementally
        // nextCurvePosition = previousCurvePosition * incrementFactor
        release.curvePosition *= release.incrementFactor;
      } else {
        transitionTo(READY);
      }
      break;

    default:
      Serial.printf("Error: Invalid Grain State");
      break;
  }

  // Create a wave if grain is active
  if(state != READY) {
    Wave wave = AudioLab.dynamicWave(grainChannel, grainFrequency, grainAmplitude, 0.0, waveType);
    windowCounter++;
  }
}

void Grain::transitionTo(grainState newState) {
  windowCounter = 0;
  state = newState;

  if (newState == READY) {
    grainFrequency = 0.0f;
    grainAmplitude = 0.0f;
  } else if (newState == ATTACK) {
    attack.curvePosition = 1.0f; // Reset attack curve progression
    release.curvePosition = 1.0f; // Reset release curve progression
  } else if (newState == SUSTAIN) {
    grainFrequency = sustain.frequency;
    grainAmplitude = sustain.amplitude;
  }
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

void Grain::shapeAttack(int duration, float freqMod, float ampMod, float curve)
{
  attack.duration = duration;
  attack.frequencyModulator = freqMod;
  attack.amplitudeModulator = ampMod;
  attack.curve = curve;
}

void Grain::shapeSustain(int duration, float freqMod, float ampMod)
{
  sustain.duration = duration;
  sustain.frequencyModulator = freqMod;
  sustain.amplitudeModulator = ampMod;
}
void Grain::shapeRelease(int duration, float freqMod, float ampMod, float curve)
{
  release.duration = duration;
  release.frequencyModulator = freqMod;
  release.amplitudeModulator = ampMod;
  release.curve = curve;
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
