#include "Grain.h"
#include <math.h>

Grain::GrainNode *Grain::globalGrainList = NULL;

void Grain::pushGrainNode(void) {
  if (globalGrainList == NULL) {
    globalGrainList = new GrainNode(this);
    return;
  }

  GrainNode *_currentNode = globalGrainList;

  while (_currentNode->next != NULL) {
    _currentNode = _currentNode->next;
  }

  _currentNode->next = new GrainNode(this);
}

Grain::Grain() {
  pushGrainNode();
  this->wave = AudioLab.staticWave(0, SINE);

  this->attackDuration = 0;
  this->attackFrequency = 0;
  this->attackAmplitude = 0;
  this->attackCurve = 1.0;
  this->attackCurveStep = 1.0;
  
  this->sustainDuration = 0;
  this->sustainFrequency = 0;
  this->sustainAmplitude = 0;

  this->releaseDuration = 0;
  this->releaseFrequency = 0;
  this->releaseAmplitude = 0;
  this->releaseCurve = 1.0;
  this->releaseCurveStep = 1.0;

  this->sustainAttackAmplitudeDifference = 0;
  this->sustainAttackFrequencyDifference = 0;
  this->releaseSustainAmplitudeDifference = 0;
  this->releaseSustainFrequencyDifference = 0;

  this->windowCounter = 0;

  this->state = READY;
}

Grain::Grain(uint8_t aChannel, WaveType aWaveType) {
  pushGrainNode();
  this->wave = AudioLab.staticWave(aChannel, aWaveType);

  this->attackDuration = 0;
  this->attackFrequency = 0;
  this->attackAmplitude = 0;
  this->attackCurve = 1.0;
  this->attackCurveStep = 1.0;
  
  this->sustainDuration = 0;
  this->sustainFrequency = 0;
  this->sustainAmplitude = 0;

  this->releaseDuration = 0;
  this->releaseFrequency = 0;
  this->releaseAmplitude = 0;
  this->releaseCurve = 1.0;
  this->releaseCurveStep = 1.0;

  this->sustainAttackAmplitudeDifference = 0;
  this->sustainAttackFrequencyDifference = 0;
  this->releaseSustainAmplitudeDifference = 0;
  this->releaseSustainFrequencyDifference = 0;

  this->windowCounter = 0;

  this->state = READY;
}

void Grain::start(void) {
  this->state = ATTACK;
  this->windowCounter = 0;
}

void Grain::stop(void) {
  this->state = READY;
  this->windowCounter = 0;
}

void Grain::setAttack(float aFrequency, float anAmplitude, int aDuration) {
  this->attackFrequency = aFrequency;
  this->attackAmplitude = anAmplitude;
  this->attackDuration = aDuration;

  this->sustainAttackFrequencyDifference = this->sustainFrequency - this->attackFrequency;
  this->sustainAttackAmplitudeDifference = this->sustainAmplitude - this->attackAmplitude;

  if (this->attackDuration > 0) this->attackCurveStep = 1.0 / this->attackDuration;
  else this->attackCurveStep = 1.0;
}

void Grain::setAttackCurve(float aCurveValue) {
  this->attackCurve = aCurveValue;
}

void Grain::setSustain(float aFrequency, float anAmplitude, int aDuration) {
  this->sustainFrequency = aFrequency;
  this->sustainAmplitude = anAmplitude;
  this->sustainDuration = aDuration;

  this->sustainAttackFrequencyDifference = this->sustainFrequency - this->attackFrequency;
  this->sustainAttackAmplitudeDifference = this->sustainAmplitude - this->attackAmplitude;

  this->releaseSustainFrequencyDifference = this->releaseFrequency - this->sustainFrequency;
  this->releaseSustainAmplitudeDifference = this->releaseAmplitude - this->sustainAmplitude;
}

void Grain::setRelease(float aFrequency, float anAmplitude, int aDuration) {
  this->releaseFrequency = aFrequency;
  this->releaseAmplitude = anAmplitude;
  this->releaseDuration = aDuration;

  this->releaseSustainFrequencyDifference = this->releaseFrequency - this->sustainFrequency;
  this->releaseSustainAmplitudeDifference = this->releaseAmplitude - this->sustainAmplitude;

  if (this->releaseDuration > 0) this->releaseCurveStep = 1.0 / this->releaseDuration;
  else this->releaseCurveStep = 1.0;
}

void Grain::setReleaseCurve(float aCurveValue) {
  this->releaseCurve = aCurveValue;
}

void Grain::setChannel(uint8_t aChannel) {
  this->wave->setChannel(aChannel);
}

void Grain::setWaveType(WaveType aWaveType) {
  AudioLab.changeWaveType(this->wave, aWaveType);
}

void Grain::run() {
  if (this->state == READY) {
    this->windowCounter = 0;
    return;
  }

  float _nowFrequency = 0;
  float _nowAmplitude = 0;
  if (this->state == ATTACK) {
    if (this->windowCounter < this->attackDuration) {
      float _curvePosition = pow(this->attackCurveStep * this->windowCounter, this->attackCurve);
      _nowFrequency = attackFrequency + sustainAttackFrequencyDifference * _curvePosition;
      _nowAmplitude = attackAmplitude + sustainAttackAmplitudeDifference * _curvePosition;
    } else {
      this->windowCounter = 0;
      this->state = SUSTAIN;
      _nowFrequency = this->sustainFrequency;
      _nowAmplitude = this->sustainAmplitude;
    }
  } else if (this->state == SUSTAIN) {
    if (!(this->windowCounter < this->sustainDuration)) {
      this->windowCounter = 0;
      this->state = RELEASE;
    }
    _nowFrequency = this->sustainFrequency;
    _nowAmplitude = this->sustainAmplitude;
  } else {
    if (this->windowCounter < this->releaseDuration) {
      float _curvePosition = pow(this->releaseCurveStep * this->windowCounter, this->releaseCurve);
      _nowFrequency = sustainFrequency + releaseSustainFrequencyDifference * _curvePosition;
      _nowAmplitude = sustainAmplitude + releaseSustainAmplitudeDifference * _curvePosition;
    } else {
      this->wave->reset();
      this->windowCounter = 0;
      this->state = READY;
      return;
    }
  }
  
  this->wave->setFrequency(_nowFrequency);
  this->wave->setAmplitude(_nowAmplitude);

  this->windowCounter += 1;
}

grainState Grain::getGrainState(void) {
  return this->state;
}

void Grain::update() {
  GrainNode* _currentNode = globalGrainList;

  while (_currentNode != NULL) {
    _currentNode->reference->run();
    _currentNode = _currentNode->next;
  }
}