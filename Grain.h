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
    int duration = 0;
    float frequency = 0;
    float amplitude = 0;
    float curve = 1.0f; // Default: linear
    float curveStep = 0.0f; // Default: no curve step
  };

  Phase attack;
  Phase sustain;
  Phase release;

  float sustainAttackAmplitudeDifference;
  float sustainAttackFrequencyDifference;
  float releaseSustainAmplitudeDifference;
  float releaseSustainFrequencyDifference;

  int windowCounter;

  float grainAmplitude;
  float grainFrequency;

  grainState state;

  Wave wave;

  //!Update frequency and amplitude values based on current grain state.
  void run();

public:
  //! Default constructor to allocate a new grain
  Grain();

   //! Overloaded constructor to allocate a new grain with specified channel 
   //! and wave type.
  Grain(uint8_t channel, WaveType waveType);

  //! Updates grain parameters in the attack state
  void setAttack(float frequency, float amplitude, int duration);

  //! Updates attack curve value.
  void setAttackCurve(float curveValue);

  //! Updates grain parameters in the sustain state
  void setSustain(float frequency, float amplitude, int duration);

  //! Updates grain parameters in the release state
  void setRelease(float frequency, float amplitude, int duration);

  //! Updates release curve value.
  void setReleaseCurve(float curveValue);

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
};

/**
 * Struct for a node in the Grain List
 */
struct GrainNode {
  //! Default constructor.
  GrainNode(Grain *object) : reference(object), next(nullptr) {}
  Grain *reference;
  GrainNode *next;
};

/**
 * Class for the management of a linked list of grains
 */
class GrainList {
private:
  GrainNode *head;
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
