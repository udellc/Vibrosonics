/**
 * @file
 * Contains the declaration of the Grain class.
*/


#ifndef Grain_h
#define Grain_h

#include <AudioLab.h>
/**
 * @type grainState
 * @brief Enum for managing the state of the current grain
*/
enum grainState {
  READY,
  ATTACK,
  SUSTAIN,
  RELEASE
};

/**
  * @brief Class for managing the generation of grains
  *
  * This class creates and manages the Ready, Attack, Sustain,
  * and Release states for individual grains. A grain is a very
  * small segment of an audio segment, allowing for more granular
  * synthesis and management of the waves that are outputted
  * through VibroSonics hardware.
  * More information about grains and Attack Sustain Decay Release
  * curves can be found at:
  * https://en.wikipedia.org/wiki/Granular_synthesis
  * https://en.wikipedia.org/wiki/Envelope_(music)
*/

class Grain {
private:
  /**
   * @brief Stuct for managing a list of grains.
   *
   * Grains are stored in a linked list for constant time read
   * access when outputting to the speakers.
   * TODO: Move globalGrainList to the api
  */
  struct GrainNode {
    GrainNode(Grain *object) : reference(object), next(NULL) {}
    Grain *reference;
    GrainNode *next;
  };

  static GrainNode *globalGrainList;

  /**
   * @brief Pushes node to the end of the grain list.
   *
   * Pushes current instance of the grain to the end of globalGrainList.
  */
  void pushGrainNode();

  // NOTE: Should these be their own structs?
  int attackDuration;
  float attackFrequency;
  float attackAmplitude;
  float attackCurve;
  float attackCurveStep;

  int sustainDuration;
  float sustainFrequency;
  float sustainAmplitude;

  int releaseDuration;
  float releaseFrequency;
  float releaseAmplitude;
  float releaseCurve;
  float releaseCurveStep;

  float sustainAttackAmplitudeDifference;
  float sustainAttackFrequencyDifference;
  float releaseSustainAmplitudeDifference;
  float releaseSustainFrequencyDifference;

  int windowCounter;

  float grainAmplitude;
  float grainFrequency;

  grainState state;

  Wave wave;

  /**
   * @brief Update frequency and amplitude values based on current grain state.
   *
   * Updates wave frequency and amplitude along with the window counter.
   * Switches grain states based on the window counter and durations for
   * each state. In essence it progresses the sample along the attack sustain
   * release curve.
   */
  void run();

public:
  /**
   * @brief Default constructor to allocate a new grain
   *
   * Creates a grain on channel 0 and sine wave type in
   * the ready state.
  */
  Grain();

  /**
   * @brief Overloaded constructor to allocate a new grain
   * with specified channel and wave type
   * 
   * Creates a grain on the specified channel and with the
   * inputted wave type in the ready state.
   *
   * @param aChannel Specified channel for output
   * @param aWaveType Specified wave type for output
  */
  Grain(uint8_t aChannel, WaveType aWaveType);

  /**
   * @brief Sets a grain to the attack state and resets
   * the window counter
  */
  void start();

  /**
   * @brief Resets grain to ready state
  */
  void stop();

  /**
   * @brief Updates grain parameters in the attack state
   *
   * Updates frequency, amplitude, and duration. Also updates sustain
   * frequency and amplitude difference.
   *
   * @param aFrequency Updated frequncy
   * @param anAmplitude Updated amplitude
   * @param aDuration Updated duration
  */
  void setAttack(float aFrequency, float anAmplitude, int aDuration);

  /**
   * @brief Updates attack curve value.
   *
   * @param aCurveValue Updated curve value
  */
  void setAttackCurve(float aCurveValue);

  /**
   * @brief Updates grain parameters in the sustain state
   *
   * Updates frequency, amplitude, and duration. Also updates attack
   * frequency and amplitude difference.
   *
   * @param aFrequency Updated frequncy
   * @param anAmplitude Updated amplitude
   * @param aDuration Updated duration
  */
  void setSustain(float aFrequency, float anAmplitude, int aDuration);

  /**
   * @brief Updates grain parameters in the release state
   *
   * Updates frequency, amplitude, and duration. Also updates attack
   * and sustain frequency and amplitude difference.
   *
   * @param aFrequency Updated frequncy
   * @param anAmplitude Updated amplitude
   * @param aDuration Updated duration
  */
  void setRelease(float aFrequency, float anAmplitude, int aDuration);

  /**
   * @brief Updates release curve value.
   *
   * @param aCurveValue Updated curve value
  */
  void setReleaseCurve(float aCurveValue);

  /**
   * @brief Sets the channel of this grain
   *
   * @param aChannel The channel on specified integer
  */
  void setChannel(uint8_t aChannel);

  /**
   * @brief Sets grain wave type (SINE, COSINE, SQUARE, TRIANGLE, SAWTOOTH)
   *
   * @param aWaveType The wave type
  */
  void setWaveType(WaveType aWaveType);

  /**
   * Returns the state of a grain (READY, ATTACK, SUSTAIN, RELEASE)
   *
   * @return grainState
  */
  grainState getGrainState();

  /**
   * @brief Updates grain values and state if necessary
   *
   * Performs the run() function on each node in the globalGrainList
  */
  static void update();

  // call these to retrieve the current amplitude or frequency of an executing grain
  float getAmplitude();
  float getFrequency();

};

#endif
