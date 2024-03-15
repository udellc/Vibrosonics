//============================================================================
// MODULE INFORMATION
//============================================================================
// Name        : Noisiness
// Return Type : float (noisiness of the current window)
// Description : Calculates the noisiness of the current window. Noisiness is
//               the opposite of periodicity, so a low noisiness value
//               indicates a high degree of periodicity, like a sine wave, and
//               a high noisiness value indicates a low degree of periodicity,
//               like white noise. 
//============================================================================
#ifndef Noisiness_h
#define Noisiness_h

#include "../AnalysisModule.h"
#include "MaxAmplitude.h"
#include "MeanAmplitude.h"

// the Noisiness module inherits from the ModuleInterface with a float output type
// this module has two submodules, MeanAmplitude and MaxAmplitude
class Noisiness : public ModuleInterface<float>
{
private:
    // submodules are made private so they cannot be accessed outside of the parent module

    // the MeanAmplitude submodule outputs the average amplitude of the frequency bins in the current window
    // average amplitude is taken as a point of comparison for the MaxAmplitude submodule
    // this is because noisier signals have a max amplitude nearer to the mean amplitude than more periodic signals
    MeanAmplitude mean = MeanAmplitude();

    // the MaxAmplitude submodule outputs the amplitude of the frequency bin with the highest amplitude
    // the MaxAmplitude submodule's output is compared against the MeanAmplitude submodule's output
    // the ratio of max to mean is used to calculate the noisiness of the signal
    MaxAmplitude max = MaxAmplitude();

public:
    // constructor
    // a constructor is necessary for modules containing submodules
    // both submodules must be registered with the parent in the constructor
    // registering submodules with a parent module allows automatic propagation of the parent's window bounds to the submodules
    Noisiness()
    {
        this->addSubmodule(&mean); // register the MeanAmplitude submodule
        this->addSubmodule(&max);  // register the MaxAmplitude submodule
    }
    
    void doAnalysis()
    {
        // perform the MeanAmplitude and MaxAmplitude analyses
        mean.doAnalysis();
        max.doAnalysis();
        
        // retrieve the results of the MeanAmplitude and MaxAmplitude analyses
        float meanAmp = mean.getOutput();
        float maxAmp = max.getOutput();

        // initialize noisiness to 0, the minimum noisiness
        float noisiness = 0.0;

        // if the mean amplitude is below 1.0, there is not enough information to meaninfully calculate noisiness
        // if the mean amplitude is below 1.0, the noisiness is defaulted to 0
        if(meanAmp > 1.0)
        {
            // calculate the noisiness of the signal as the ratio of the max amplitude to the mean amplitude
            // the ratio is scaled to a range of 0 to 1, where 0 is the least noisy and 1 is the most noisy
            noisiness = 1 - ((maxAmp / meanAmp) / 66.7);
        }
        
        // store the noisiness in the output variable
        // the output of this module can be retrieved by calling getOutput() after analysis
        output = noisiness;
    }
};

#endif