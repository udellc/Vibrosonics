#ifndef Salient_Freqs_h
#define Salient_Freqs_h

#include "AnalysisModule.h"
#include <algorithm>

//  used to find the n (numFreqs) bin indexes with the highest change in amplitude
class SalientFreqs :: public AnalysisModule<int*>
{
  public:
    int numFreqs;
    int* salientFreqs;
    DeltaAmplitudes deltaAmps;

    SalientFreqs()
    {
      int numFreqs = 1; // default to finding frequency of max change
      salientFreqs = new int[numFreqs];
    }
    ~SalientFreqs()
    {
      delete [] salientFreqs;
    }

    // change the amount of salient frequencies to be found
    void changeNumFreqs(int newSize)
    {
      numFreqs = newSize;
      delete [] salientFreqs;
      salientFreqs = new int[numFreqs];
    }

    // finds the n (numFreqs) bins with highest change in amplitude, stored in salientFreqs[]
    void doAnalysis()
    {
      deltaAmps.doAnalysis();
      int* amplitudes = deltaAmps.getOutput();

      // iterate through amplitudes to find the maximum(s)
      int currMaxAmp = 0; 
      int currMaxAmpIdx = -1;
      for (int i = 0; i < numFreqs; i++) {
        for (int j = 0; j < windowSize>>1; j++) {
          if (amplitudes[j] > currMaxAmp) {
            currMaxAmp = amplitudes[j];
            currMaxAmpIdx = j;
          }
        }
        salientFreqs[i] = currMaxAmpIdx;  // add the new max amp index to the array
        amplitudes[j] = 0;  // set the amp to 0 so it will not be found again
        currMaxAmp = 0; // reset iterators
        currMaxAmpIdx = -1;
      } 
    }
};

#endif