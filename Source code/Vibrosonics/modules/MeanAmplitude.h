//============================================================================
// MODULE INFORMATION
//============================================================================
// Name        : MeanAmplitude
// Return Type : float (mean amplitude of the freq. bins in the current window)
// Description : Returns the mean amplitude of the frequency bins in the 
//               current window. If a frequency range is specified, the module 
//               will only consider the bins within the specified range.
//============================================================================

#ifndef Mean_Amplitude_h
#define Mean_Amplitude_h

#include "../AnalysisModule.h"
#include "TotalAmplitude.h"

// MeanAmplitude inherits from the ModuleInterface with a float output type
class MeanAmplitude : public ModuleInterface<float>
{
private:
    // private instance of the TotalAmplitude module
    // this module is used to sum the amplitudes of all the frequency bins
    TotalAmplitude totalAmp = TotalAmplitude();

public:
    
    // a constructor must be implemented to register submodules
    MeanAmplitude()
    {
        // register TotalAmplitude as a submodule of MeanAmplitude
        this->addSubmodule(&totalAmp);
    }
    
    void doAnalysis()
    {
        // run the TotalAmplitude module and retrieve its output
        totalAmp.doAnalysis();
        float total = totalAmp.getOutput();

        // output the mean amplitude
        output = total / (upperBinBound - lowerBinBound);
    }
};
#endif