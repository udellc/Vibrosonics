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
    
    // doAnalysis() is called by the analysis manager
    // it finds the frequency bin with the highest amplitude in the current window
    // the max amplitude is stored in the module's output variable
    void doAnalysis(const float** input)
    {
        // initialize max to 0 (there cannot be negative amplitudes)
        float max = 0.0;
        
        // iterate through the bins in the selected frequency range
        // if a bin's amplitude is greater than max, update max
        // at the end of the loop, max will contain the amplitude of the highest bin
        for(int i=lowerBinBound; i<upperBinBound; i++)
        {
            if(input[CURR_WINDOW][i] > max){ max = input[CURR_WINDOW][i]; }
        }

        // store the max amplitude in the output variable
        // the output of this module can be retrieved by calling getOutput() after analysis
        output = max;
    }
};

#endif