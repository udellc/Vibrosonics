//============================================================================
// MODULE INFORMATION
//============================================================================
// Name        : TotalAmplitude
// Return Type : float
// Description : Returns the sum of the amplitudes of the frequency bins in 
//               the current window. If a frequency range is specified, the 
//               module will only consider the bins within the specified range.
//============================================================================

#ifndef Total_Amplitude_h
#define Total_Amplitude_h

#include "../AnalysisModule.h"

// TotalAmplitude inherits from the ModuleInterface with a float output type
class TotalAmplitude : public ModuleInterface<float>
{
public:
    void doAnalysis()
    {
        float total = 0.0;
        
        // sum the amplitudes of the frequency bins within the specified range
        for(int i=lowerBinBound; i<upperBinBound; i++)
        {
          total += curWindow[i];
        }

        // output the sum of the amplitudes
        output = total;
    }
};

#endif