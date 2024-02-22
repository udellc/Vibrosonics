#ifndef Delta_Amplitudes_h
#define Delta_Amplitudes_h

#include "../AnalysisModule.h"
#include <cmath>

/*  Used to find the change in amplitudes between the current and previous FFT window
    for each bin */ 
class DeltaAmplitudes : public ModuleInterface<float*>
{
  public:
    float* deltaAmplitudes; 

    DeltaAmplitudes()
    {
      deltaAmplitudes = new float[windowSize];
    } 
    ~DeltaAmplitudes()
    {
      delete [] deltaAmplitudes;
    }

    void doAnalysis()
    {
      // iterate through FFT data and store the change in amplitudes between current and previous window
      for (int i = lowerBinBound; i < upperBinBound; i++) {
        deltaAmplitudes[i] = abs(curWindow[i] - pastWindow[i]);
      }

      output = deltaAmplitudes;
    }
};

#endif