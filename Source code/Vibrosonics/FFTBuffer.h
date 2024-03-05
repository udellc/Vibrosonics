#ifndef FFTBuffer_h
#define FFTBuffer_h

#include <Arduino.h>
#include <math.h>

class FFTBuffer 
{

  private:
    int windowSize;
    int sampleRate;

    int bufferSize;

    float frequencyWidth;

    float **timeFrequencyWindows;
    int timeFrequencyWindowCount;

  public:
    FFTBuffer(int aWindowSize, int aSampleRate, int aNumberOfWindows);
    ~FFTBuffer();

    // push FFT data
    void pushData(float *someFFTData);

    // returns double pointer to FFT data buffer
    float **getData();

    // returns a window based off index
    float *getWindow(int aWindowIndex);

    // returns index of current window
    int getCurrentWindowIndex();

};

#endif