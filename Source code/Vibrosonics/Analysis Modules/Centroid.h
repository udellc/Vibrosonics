#ifndef Centroid_h
#define Centroid_h

#include "AnalysisModule.h"
#include <cmath>

//  finds the weighted mean of the frequencies present, using amplitude as weights
class Centroid : public AnalysisModule<int>
{
  public:
    int centroid;
    int freqWidthBy2 = freqWidth/2;

    void doAnalysis()
    {
      // get the sum of amplitudes and sum of frequencies*amplitudes
      float ampSum = 0;
      float freqAmpSum = 0;
      int amp, freq;
      for (int i = 0; i < windowSize>>1; i++) {
        amp = input[0][i];
        freq = ((i+1) * freqWidth - freqWidthBy2);  // use the center frequency of a bin
        ampSum += amp;
        freqAmpSum += freq * amp;
      }
      centroid = std::ceil(freqAmpSum / ampSum);
      output = centroid;
    }
};

#endif