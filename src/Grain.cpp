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
  windowCounter = 0;
  isDynamic = false;
  markedForDeletion = false;
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
  windowCounter = 0;
  this->isDynamic = false;
  markedForDeletion = false;
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
}

/**
 * Updates frequency, amplitude, and duration for DECAY.
 *
 * @param frequency Updated frequncy
 * @param amplitude Updated amplitude
 * @param duration Updated duration
 */
void Grain::setDecay(float frequency, float amplitude, int duration)
{
  decay.frequency = frequency;
  decay.amplitude = amplitude;
  decay.duration = duration;
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
      break;

    case ATTACK:
      if (windowCounter < attack.duration) {
        float pos  = (float)(windowCounter + 1) / (float)attack.duration;
        float curvedProgress = powf(pos, attack.curve);
        // Compute and update frequncy and amplitude with incremental curve position
        grainFrequency = attack.frequency * curvedProgress;
        grainAmplitude = attack.amplitude * curvedProgress;

      } else {
        transitionTo(DECAY);
      }
      break;

    case DECAY:
      if (windowCounter < decay.duration) {
        float pos  = (float)(windowCounter + 1) / (float)decay.duration;
        float curvedProgress = powf(pos, decay.curve);
        grainFrequency = decay.frequency + (sustain.frequency - attack.frequency) * curvedProgress;
        grainAmplitude = decay.amplitude + (sustain.amplitude - attack.amplitude) * curvedProgress;
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
        grainFrequency = sustain.frequency + (release.frequency - sustain.frequency) * curvedProgress;
        grainAmplitude = sustain.amplitude + (release.amplitude - sustain.amplitude) * curvedProgress;
      } else {
        transitionTo(READY);
      }
      break;

    default:
      // If you're seeing this, you've done something wrong
      Serial.printf("Error: Invalid Grain State");
      break;
  }

  // Create a wave if grain is active
  if(state != READY) {
    AudioLab.dynamicWave(grainChannel, grainFrequency, grainAmplitude, 0.0, waveType);
    windowCounter++;
  }
  this->printGrain();
}

/**
 * Helper function for run. Handles skipped states and prepares Grain
 * for the current window without leaving a 1 window gap between grain states.
 *
 * @param newState State to transition to.
 */
void Grain::transitionTo(grainState newState) {
  windowCounter = 0;
  state = newState;
  bool stateSkipped;

  do {
    stateSkipped = false;
    switch (state) {
      case READY:
        if (isDynamic) {
          markedForDeletion = true;
        }
        grainFrequency = 0.0f;
        grainAmplitude = 0.0f;
        break;
      case ATTACK:
        if (attack.duration <= 0) {
          state = DECAY;
          stateSkipped = true;
        } else {
          float pos  = (float)(windowCounter + 1) / (float)attack.duration;
          float curvedProgress = powf(pos, attack.curve);
          grainFrequency = attack.frequency * curvedProgress;
          grainAmplitude = attack.amplitude * curvedProgress;
        }
        break;
      case DECAY:
        if (decay.duration <= 0) {
          state = SUSTAIN;
          stateSkipped = true;
        } else {
          float pos  = (float)(windowCounter + 1) / (float)decay.duration;
          float curvedProgress = powf(pos, decay.curve);
          grainFrequency = decay.frequency + (sustain.frequency - attack.frequency) * curvedProgress;
          grainAmplitude = decay.amplitude + (sustain.amplitude - attack.amplitude) * curvedProgress;
        }
        break;
      case SUSTAIN:
        if (sustain.duration <= 0) {
          state = RELEASE;
          stateSkipped = true;
        } else {
          grainFrequency = sustain.frequency;
          grainAmplitude = sustain.amplitude;
        }
        break;
      case RELEASE:
        if (release.duration <= 0) {
          state = READY;
          stateSkipped = true;
        } else {
          float pos = (float)(windowCounter + 1) / (float)release.duration;
          float curvedProgress = powf(pos, release.curve);
          // Compute and update frequncy and amplitude with incremental curve position
          grainFrequency = sustain.frequency + (release.frequency - sustain.frequency) * curvedProgress;
          grainAmplitude = sustain.amplitude + (release.amplitude - sustain.amplitude) * curvedProgress;
        }
        break;
      default:
        Serial.printf("Error: Invalid GrainState to transition to\n");
        break;
    }
  } while(stateSkipped);
}

/**
 * Returns the state of a grain (READY, ATTACK, DECAY, SUSTAIN, RELEASE)
 *
 * @return grainState
 */
grainState Grain::getGrainState()
{
  return state;
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
 * Returns the decay window duration
 *
 * @return int
 */
int Grain::getDecayDuration()
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
 * Sets state parameters based on the frequency envelope passed in.
 *
 * @param freqEnv Frequency envelope containing new data to update state parameters with.
 */
void Grain::setFreqEnv(FreqEnv freqEnv)
{
  attack.frequency = freqEnv.targetFrequency;
  decay.frequency = freqEnv.targetFrequency;
  sustain.frequency = freqEnv.targetFrequency;
  release.frequency = freqEnv.minFrequency;
}

/**
 * Sets state parameters based on the amplitude envelope passed in.
 *
 * @param freqEnv Amplitude envelope containing new data to update state parameters with.
 */
void Grain::setAmpEnv(AmpEnv ampEnv)
{
  attack.amplitude = ampEnv.targetAmplitude;
  attack.duration = ampEnv.attackDuration;
  attack.curve = ampEnv.curve;
  decay.amplitude = ampEnv.targetAmplitude;
  decay.duration = ampEnv.decayDuration;
  decay.curve = ampEnv.curve;
  sustain.amplitude = ampEnv.targetAmplitude * .8;
  sustain.duration = ampEnv.sustainDuration;
  sustain.curve = ampEnv.curve;
  release.amplitude = ampEnv.minAmplitude;
  release.duration = ampEnv.releaseDuration;
  release.curve = ampEnv.curve;
}

/**
 * Prints grain debug info.
 */
void Grain::printGrain()
{
  Serial.printf("State: %i, Frequency: %f, Amplitude: %f\n", state, grainFrequency, grainAmplitude);
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
 * Runs update on all grains in the list. Deletes dynamic grains if they have
 * finished their lifespan and are ready to be "reaped".
 */
void GrainList::updateAndReap()
{
  GrainNode *current = head;
  GrainNode *prev = nullptr;

  while (current != nullptr) {
    GrainNode *nextNode = current->next;
    current->reference->run();
    if (current->reference->isDynamic &&
      current->reference->markedForDeletion &&
      current->reference->getGrainState() == READY
    ) {
      if (prev == nullptr) {
        head = nextNode;
      } else {
        prev->next = nextNode;
      }
      if (nextNode == nullptr) {
        tail = prev;
      }

      delete current->reference;
      delete current;

      current = nextNode;
    } else {
      prev = current;
      current = nextNode;
    }
  }
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
