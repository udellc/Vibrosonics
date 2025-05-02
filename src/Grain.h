/**
 * @file
 * Contains the declaration of the Grain class.
*/

#ifndef Grain_h
#define Grain_h

#include <AudioLab.h>

/**
 * @type grainState
 *
 * Enum for managing the state of the current grain
*/
enum grainState {
  READY,
  ATTACK,
  SUSTAIN,
  RELEASE
};

class GrainList;

/**
  * This class creates and manages the Ready, Attack, Sustain,
  * and Release states for individual grains. A grain is a very
  * small segment of an audio segment, allowing for more granular
  * synthesis and management of the waves that are outputted
  * through VibroSonics hardware.
  *
  * More information about grains and Attack Sustain Decay Release
  * curves can be found at:
  *
  * https://en.wikipedia.org/wiki/Granular_synthesis
  *
  * https://en.wikipedia.org/wiki/Envelope_(music)
  */
class Grain {
private:
  struct Phase {
    //! Number of windows each phase runs for
    int duration = 0;
    //! Targeted frequency for the phase
    float frequency = 0.0f;
    //! Targeted amplitude for the phase
    float amplitude = 0.0f;
    //! Modulates frequency by multiplying frequencyModulator to frequency
    float frequencyModulator = 1.0f; // Default: no modulation
    //! Modulates amplitude by multiplying amplitudeModulator to amplitude
    float amplitudeModulator = 1.0f; // Default: no modulation

    //! Shapes the curve by raising curveStep*windowCounter to the degree of the curve value
    float curve = 1.0f; // Default: linear
  };

  //! Struct containing attack parameters
  Phase attack;
  //! Struct containing sustain parameters
  Phase sustain;
  //! Struct containing release parameters
  Phase release;

  //! The difference in frequency between sustain and attack
  float sustainAttackFrequencyDifference;
  //! The difference in frequency between release and sustain
  float releaseSustainFrequencyDifference;

  //! The counter for how many windows a state has run for
  int windowCounter;

  //! The current amplitude of the grain
  float grainAmplitude;
  //! The current frequency of the grain
  float grainFrequency;

  //! The wave shape of outputted grain waves
  WaveType waveType;

  //! The output channel for generated grain waves
  uint8_t grainChannel;

  //! The current envelope state of the grain
  grainState state;

  //! Update frequency and amplitude values based on current grain state.
  void run();

  //! Helper function to perform necessary operations on grain parameters
  //! when transitioning between run states
  void transitionTo(grainState newState);

public:
  //! Default constructor to allocate a new grain
  Grain();

   //! Overloaded constructor to allocate a new grain with specified channel 
   //! and wave type.
  Grain(uint8_t channel, WaveType waveType);

  //! Updates grain parameters in the attack state
  void setAttack(float frequency, float amplitude, int duration);

  //! Updates grain parameters in the sustain state
  void setSustain(float frequency, float amplitude, int duration);

  //! Updates grain parameters in the release state
  void setRelease(float frequency, float amplitude, int duration);

  //! Sets the channel of this grain
  void setChannel(uint8_t channel);

  //! Sets grain wave type (SINE, COSINE, SQUARE, TRIANGLE, SAWTOOTH)
  void setWaveType(WaveType waveType);

  //! Returns the state of a grain (READY, ATTACK, SUSTAIN, RELEASE)
  grainState getGrainState();

  //! Updates grain values and state if necessary
  static void update(GrainList *globalGrainList);

  //! Returns the current amplitude of the grain
  float getAmplitude();

  //! Returns the current frequency of the grain
  float getFrequency();

  //! Returns the attack duration
  int getAttackDuration();

  //! Returns the sustain duration
  int getSustainDuration();

  //! Returns the release duration
  int getReleaseDuration();

  //! Sets parameters for the attack state
  void shapeAttack(int duration, float freqMod, float ampMod, float curve);

  //! Sets parameters for the sustain state
  void shapeSustain(int duration, float freqMod, float ampMod);

  //! Sets parameters for the release state
  void shapeRelease(int duration, float freqMod, float ampMod, float curve);
};

/**
 * Struct for a node in the Grain List
 */
struct GrainNode {
  //! Default constructor.
  GrainNode(Grain *object) : reference(object), next(nullptr) {}
  //! Reference containing grain data
  Grain *reference;
  //! Pointer to the next grain in the list
  GrainNode *next;
};

/**
 * Class for the management of a linked list of grains
 */
class GrainList {
private:
  //! Grain at the front of the list
  GrainNode *head;
  //! Grain at the back of the list
  GrainNode *tail;
public:
  //! Default constructor.
  GrainList() : head(nullptr), tail(nullptr) {}
  //! Pushes a grain to the tail of the list.
  void pushGrain(Grain *grain);
  //! Deletes all grains in the list.
  void clearList();
  //! Returns the head of the list.
  GrainNode* getHead();
};

#endif
