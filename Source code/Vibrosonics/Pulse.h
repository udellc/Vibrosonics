#ifndef Pulse_h
#define Pulse_h

#include <AudioLab.h>

enum pulseState {
  READY,
  ATTACK,
  SUSTAIN,
  RELEASE
};

class Pulse {
  private:
    struct PulseNode {
      PulseNode(Pulse *object) : reference(object), next(NULL) {}
      Pulse *reference;
      PulseNode *next;
    };

    static PulseNode *globalPulseList;

    void pushPulseNode();

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

    pulseState state;

    Wave wave;

    void run();

  public:

    // Pulse object constructor
    Pulse(uint8_t aChannel, WaveType aWaveType);

    // Begin pulsing, will do a single pulse with set parameters
    void start();

    // set Pulse attack parameters, will transition to Sustain parameters over a given duration
    void setAttack(float aFrequency, float anAmplitude, int aDuration);
    // set the curve to follow when transitioning from attack parameters to sustain parameters
    void setAttackCurve(float aCurveValue);

    // set Pulse sustain parameters, will sustain the frequency and amplitude over a given duration
    void setSustain(float aFrequency, float anAmplitude, int aDuration);
    
    // set Pulse release parameters, will transition from sustain parameters over a given duration
    void setRelease(float aFrequency, float anAmplitude, int aDuration);
    // set the curve to follow when transtioning from sustain parameters to release parameters
    void setReleaseCurve(float aCurveValue);

    // set channel of pulse
    void setChannel(uint8_t aChannel);

    // set pulse wave type (SINE, COSINE, SQUARE, TRIANGLE, SAWTOOTH)
    void setWaveType(WaveType aWaveType);

    // returns the state of a pulse (READY, ATTACK, SUSTAIN, RELEASE)
    pulseState getPulseState();

    // call this function in AudioLab.ready() block
    static void update();

};

#endif