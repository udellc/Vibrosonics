//============================================================================
// MODULE INFORMATION
//============================================================================
// Name        : MaxAmplitude
// Return Type : float (amplitude of the freq. bin with the highest amplitude)
// Description : Returns the amplitude of the frequency bin with the highest 
//               amplitude in the current window. If a frequency range is 
//               specified, the module will only consider the bins within the 
//               specified range.
//============================================================================

#ifndef Max_Amplitude_h
#define Max_Amplitude_h

#include "../AnalysisModule.h"

// MaxAmplitude inherits from the ModuleInterface with a float output type
class MaxAmplitude : public ModuleInterface<float>
{
public:
    void doAnalysis()
    {
        float max = 0.0;
        
        // search within the specified frequency range
        for(int i=lowerBinBound; i<upperBinBound; i++)
        {
            if(curWindow[i] > max){ max = curWindow[i]; }
        }

        // set the output to the max amplitude
        output = max;
    }
};

#endif