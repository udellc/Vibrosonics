#ifndef Delta_Amplitudes_h
#define Delta_Amplitudes_h

#include "AnalysisModule.h"
#include <cmath>

/*  Used to find the change in amplitudes between the current and previous FFT window
    for each bin */ 
class DeltaAmplitudes : public AnalysisModule<int*>
{
  public:
    int* deltaAmplitudes; 

    DeltaAmplitudes()
    {
      deltaAmplitudes = new int[windowSize];
    } 
    ~DeltaAmplitudes()
    {
      delete [] deltaAmplitudes;
    }

    void doAnalysis()
    {
      // iterate through FFT data and store the change in amplitudes between current and previous window
      int ampChange = 0;
      for (int i = 0; i < windowSize>>1; i++) {
        deltaAmplitudes[i] = abs(int(input[0][i]) - int(input[1][i])) 
      }

      output = deltaAmplitudes;
    }
};

#endif