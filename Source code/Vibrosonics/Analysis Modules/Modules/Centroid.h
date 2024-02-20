#ifndef Centroid_h
#define Centroid_h

#include "../Analysis Module/AnalysisModule.h"
#include <cmath>

//  finds the weighted mean of the frequencies present, using amplitude as weights
class Centroid : public ModuleInterface<int>
{
  public:
    int centroid;
    int freqResBy2 = freqRes/2;

    void doAnalysis()
    {
      // get the sum of amplitudes and sum of frequencies*amplitudes
      float ampSum = 0;
      float freqAmpSum = 0;
      int amp, freq;
      for (int i = lowerBinBound; i < upperBinBound; i++) {
        amp = curWindow[i];
        freq = (i * freqRes + freqResBy2);  // use the center frequency of a bin
        ampSum += amp;
        freqAmpSum += freq * amp;
      }
      centroid = std::ceil(freqAmpSum / ampSum);
      output = centroid;
    }
};

#endif