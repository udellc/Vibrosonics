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

// MeanAmplitude inherits from the ModuleInterface with a float output typ
// this module contains one submodule, TotalAmplitude
class MeanAmplitude : public ModuleInterface<float>
{
private:
    // submodules are made private so they cannot be accessed outside of the parent module
    // submodules must be registered with their parents in a constructor method
    
    // this TotalAmplitude submodule is used to calculate the sum of bin amplitudes in the current window
    // it's doAnalysis() method is called from the parent module's doAnalysis() method
    // the output of the submodule is used to calculate the mean amplitude
    TotalAmplitude totalAmp = TotalAmplitude();

public:
    // constructor
    // a constructor is necessary for modules containing submodules
    // the submodule must be registered with the parent in the constructor
    // registering a submodule with a parent module allows automatic propagation of the parent's window bounds to the submodul
    MeanAmplitude()
    {
        this->addSubmodule(&totalAmp);
    }
    
    // doAnalysis() is called by the analysis manager
    // the totalamplitude submodule is invoked to calculate the total amplitude of the current window
    // the mean amplitude is calculated from the total amplitude and the number of bins in the selected frequency range
    void doAnalysis(const float** input)
    {
        // perform analysis on the totalamplitude module
        totalAmp.doAnalysis(input);

        // retrieve the output of the totalamplitude module
        float total = totalAmp.getOutput();
        
        // calculate the mean amplitude by dividing the total amplitude by the number of bins in the selected frequency range
        // this module's output can be retrieved by calling getOutput() after analysis
        output = total / (upperBinBound - lowerBinBound);
    }
};
#endif