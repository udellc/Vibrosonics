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
    // doAnalysis() is called by the analysis manager
    // it finds the sum of the amplitudes of the bins in the selected frequency range
    // the sum is stored in the module's output variable
    // input is a 2D array that contains the stored FFT history
    void doAnalysis(const float** input)
    {
        // initialize total to 0, the minimum possible amplitude sum
        float total = 0.0;
        
        // iterate through the bins in the selected frequency range, adding the amplitude of each bin to the total
        for(int i=lowerBinBound; i<upperBinBound; i++)
        {
          //total += curWindow[i];
          total += input[CURR_WINDOW][i];
        }

        // store the total amplitude in the output variable
        // the output of this module can be retrieved by calling getOutput() after analysis
        output = total;
    }
};

#endif