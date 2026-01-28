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
  DECAY,
  SUSTAIN,
  RELEASE
};

/**
 * @struct FreqEnv
 *
 * Struct containing target frequency data for a grain
 *
 * @var FreqEnv::attackFrequency
 * The maximum frequency the grain will reach in its attack state.
 * @var FreqEnv::decayFrequency
 * The maximum frequency the grain will reach in its decay state.
 * @var FreqEnv::sustainFrequency
 * The frequency the grain will output in its sustain state.
 * @var FreqEnv::releaseFrequency
 * The lowest frequency the grain will output in its release state.
 */
struct FreqEnv {
  float attackFrequency = 100.0;
  float decayFrequency = 100.0;
  float sustainFrequency = 100.0;
  float releaseFrequency = 100.0;
};

/**
 * @struct AmpEnv
 *
 * Struct containing target amplitude data for a grain, its state
 * duration parameters, and the curve shape.
 *
 * @var AmpEnv::attackAmplitude
 * The maximum amplitude the grain will reach in its attack state.
 * @var AmpEnv::decayAmplitude
 * The maximum amplitude the grain will reach in its decay state.
 * @var AmpEnv::sustainAmplitude
 * The amplitude the grain will output in its sustain state.
 * @var AmpEnv::releaseAmplitude
 * The minimum amplitude the grain will reach in its release state.
 */
struct AmpEnv {
  float attackAmplitude = 0.5;
  int attackDuration = 1;
  float decayAmplitude = 0.5;
  int decayDuration = 1;
  float sustainAmplitude = 0.5;
  int sustainDuration = 1;
  float releaseAmplitude = 0.0;
  int releaseDuration = 1;
  float curve = 1.0f;
};

/**
 * @struct DurEnv
 *
 * Struct containing window durations for a grain
 *
 * @var AmpEnv::attackDuration
 * The number of windows the attack state will run for.
 * @var AmpEnv::decayDuration
 * The number of windows the decay state will run for.
 * @var AmpEnv::sustainDuration
 * The number of windows the sustain state will run for.
 * @var AmpEnv::releaseDuration
 * The number of windows the release state will run for.
 * @var AmpEnv::curve
 * The shape of the progression through the ADSR curve.
 */
struct DurEnv {
  int attackDuration = 1;
  int decayDuration = 1;
  int sustainDuration = 1;
  int releaseDuration = 1;
  float curve = 1.0f;
};

class GrainList;

/**
  * This class creates and manages the Ready, Attack, Decay, Sustain,
  * and Release states for individual grains. A grain is a very
  * small segment of an audio sample, allowing for more granular
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
    //! Shapes the curve by raising curveStep*windowCounter to the degree of the curve value
    float curve = 1.0f; // Default: linear
  };

  //! Struct containing attack parameters
  Phase attack;
  //! Struct containing decay parameters
  Phase decay;
  //! Struct containing sustain parameters
  Phase sustain;
  //! Struct containing release parameters
  Phase release;

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
public:
  //! Flag to check if a grain is dynamic or static.
  bool isDynamic;
  //! Flag to check if a dynamic grain has finished triggering.
  bool markedForDeletion;

  //! Default constructor to allocate a new grain
  Grain();

   //! Overloaded constructor to allocate a new grain with specified channel 
   //! and wave type.
  Grain(uint8_t channel, WaveType waveType);

  //! Updates grain parameters in the attack state
  void setAttack(float frequency, float amplitude, int duration);

  //! Updates grain parameters in the decay state
  void setDecay(float frequency, float amplitude, int duration);

  //! Updates grain parameters in the sustain state
  void setSustain(float frequency, float amplitude, int duration);

  //! Updates grain parameters in the release state
  void setRelease(float frequency, float amplitude, int duration);

  //! Sets the channel of this grain
  void setChannel(uint8_t channel);

  //! Sets grain wave type (SINE, COSINE, SQUARE, TRIANGLE, SAWTOOTH)
  void setWaveType(WaveType waveType);

  //! Returns the state of a grain (READY, ATTACK, DECAY, SUSTAIN, RELEASE)
  grainState getGrainState();

  //! Helper function to perform necessary operations on grain parameters
  //! when transitioning between run states
  void transitionTo(grainState newState);

  //! Returns the current amplitude of the grain
  float getAmplitude();

  //! Returns the current frequency of the grain
  float getFrequency();

  //! Returns the attack duration
  int getAttackDuration();

  //! Returns the decay duration
  int getDecayDuration();

  //! Returns the sustain duration
  int getSustainDuration();

  //! Returns the release duration
  int getReleaseDuration();

  //! Sets grain parameters for the frequency envelope
  void setFreqEnv(FreqEnv freqEnv);

  //! Sets grain parameters for the amplitude envelope
  void setAmpEnv(AmpEnv ampEnv);

  //! Sets grain window duration for the duration envelope
  void setDurEnv(DurEnv durEnv);

  //! Returns the frequncy envelope struct containing state data
  FreqEnv getFreqEnv();

  //! Returns the amplitude envelope struct containing state data
  AmpEnv getAmpEnv();

  //! Returns the duration envelope struct containing duration data
  DurEnv getDurEnv();

  //! For debugging: Prints a grain's state, frequency, and amplitude.
  void printGrain();

  friend class GrainList;
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
  //! Updates grains and deletes finished dynamic grains.
  void updateAndReap();
};
#endif
