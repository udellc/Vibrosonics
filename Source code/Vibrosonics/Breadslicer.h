#ifndef Breadslicer_h
#define Breadslicer_h

#include <Arduino.h>

class Breadslicer
{

  private:
    int windowSize;
    int sampleRate;
    float frequencyWidth;

    int *bandIndexes;
    int numBands;

    float *output;

  public:

    Breadslicer(int aWindowSize, int aSampleRate);
    ~Breadslicer();

    void setBands(int *frequencyBands, int numBands);
    void perform(float *input);
    float *getOutput();

};

#endif