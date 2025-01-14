#ifndef Grain_h
#define Grain_h

#include <AudioLab.h>
/**
 * @type grainState
 * Enum for managing the state of the current grain
*/
enum grainState {
  READY,
  ATTACK,
  SUSTAIN,
  RELEASE
};

class Grain {
private:
  struct GrainNode {
    GrainNode(Grain *object) : reference(object), next(NULL) {}
    Grain *reference;
    GrainNode *next;
  };

  static GrainNode *globalGrainList;

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

  // grainAmplitude and grainFrequency do not affect the output waves
  // they are only used to report frequency and amplitude values
  // SEE: getAmplitude() and getFrequency()
  // NOTE: I have no idea what they are talking about. Those getters tell you nothing.
  float grainAmplitude;
  float grainFrequency;

  grainState state;

  Wave wave;

  void run();

public:

  Grain();

  // Grain object constructor
  Grain(uint8_t aChannel, WaveType aWaveType);

  // Begin pulsing, will do a single grain with set parameters
  void start();

  void stop();

  // set Grain attack parameters, will transition to Sustain parameters over a given duration
  void setAttack(float aFrequency, float anAmplitude, int aDuration);
  // set the curve to follow when transitioning from attack parameters to sustain parameters
  void setAttackCurve(float aCurveValue);

  // set Grain sustain parameters, will sustain the frequency and amplitude over a given duration
  void setSustain(float aFrequency, float anAmplitude, int aDuration);

  // set Grain release parameters, will transition from sustain parameters over a given duration
  void setRelease(float aFrequency, float anAmplitude, int aDuration);
  // set the curve to follow when transtioning from sustain parameters to release parameters
  void setReleaseCurve(float aCurveValue);

  // set channel of grain
  void setChannel(uint8_t aChannel);

  // set grain wave type (SINE, COSINE, SQUARE, TRIANGLE, SAWTOOTH)
  void setWaveType(WaveType aWaveType);

  // returns the state of a grain (READY, ATTACK, SUSTAIN, RELEASE)
  grainState getGrainState();

  // call this function in AudioLab.ready() block
  static void update();

  // call these to retrieve the current amplitude or frequency of an executing grain
  float getAmplitude();
  float getFrequency();

};

#endif
