//============================================================================
// MODULE INFORMATION
//============================================================================
// Name        : DeltaAmplitudes
// Return Type : float* (list of ampltidue deltas, indexed by frequency bin)
// Description : Used to find the change in amplitudes between the current and
//               previous FFT window for each bin.
//============================================================================

#ifndef Delta_Amplitudes_h
#define Delta_Amplitudes_h

#include "../AnalysisModule.h"
#include <cmath>

// DeltaAmplitudes inherits from the ModuleInterface with a float* output type
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

    void doAnalysis(const float** input)
    {
      // iterate through FFT data and store the change in amplitudes between current and previous window
      for (int i = lowerBinBound; i < upperBinBound; i++) {
        deltaAmplitudes[i] = abs(input[CURR_WINDOW][i] - input[PREV_WINDOW][i]);
      }

      output = deltaAmplitudes;
    }

    void printOutput()
    {
        Serial.printf("Delta Amplitudes: ");
        for(int i=lowerBinBound; i<upperBinBound; i++){
            Serial.printf("%01g, ", round(output[i]));
        }    
        Serial.printf("\n");
    }
};

#endif