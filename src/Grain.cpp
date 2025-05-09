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
  sustainAttackFrequencyDifference = 0;
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
  sustainAttackFrequencyDifference = 0;
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

  releaseSustainFrequencyDifference = release.frequency - sustain.frequency;
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
        float pos  = (float)(windowCounter + 1) / (float)attack.duration;
        float curvedProgress = powf(pos, attack.curve);
        // Compute and update frequncy and amplitude with incremental curve position
        grainFrequency = attack.frequency + sustainAttackFrequencyDifference * attack.frequencyModulator;
        grainAmplitude = attack.amplitude * curvedProgress;

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
        float pos = (float)(windowCounter + 1) / (float)release.duration;
        float curvedProgress = powf(pos, release.curve);
        // Compute and update frequncy and amplitude with incremental curve position
        grainFrequency = sustain.frequency + releaseSustainFrequencyDifference * release.frequencyModulator;
        grainAmplitude = sustain.amplitude + (release.amplitude - sustain.amplitude) * curvedProgress;
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
    float pos  = (float)(windowCounter + 1) / (float)attack.duration;
    float curvedProgress = powf(pos, attack.curve);
    grainFrequency = attack.frequency + sustainAttackFrequencyDifference * attack.frequencyModulator;
    grainAmplitude = attack.amplitude * curvedProgress;
  } else if (newState == SUSTAIN) {
    grainFrequency = sustain.frequency * sustain.frequencyModulator;
    grainAmplitude = sustain.amplitude * sustain.amplitudeModulator;
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
