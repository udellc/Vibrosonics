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
//
//               The noisiness of the current window is the
//               entropy of the normalized amplitude spectrum. This is a
//               measure of the amount of the randomness / unpredictability in
//               the current window, treating the normalized amplitude
//               spectrum as a probability distribution.
//============================================================================

#ifndef Noisiness_h
#define Noisiness_h

#include <math.h>

#include "../AnalysisModule.h"
#include "MaxAmplitude.h"
#include "MeanAmplitude.h"

// the Noisiness module inherits from the ModuleInterface with a float output type
// this module has one submodule, TotalAmplitude
class Noisiness : public ModuleInterface<float>
{
private:
    // submodules are made private so they cannot be accessed outside of the parent module

    // the TotalAmplitude submodule outputs the sum of the amplitudes of the frequency bins in the current window
    // the TotalAmplitude submodule is used to normalize the bin amplitudes
    // normalized bin amplitudes are necessary for calculating the entropy of the current window
    TotalAmplitude totalAmp = TotalAmplitude();

public:
    // constructor
    // a constructor is necessary for modules containing submodules
    // submodules must be registered with the parent in the constructor
    // registering submodules with a parent module allows automatic propagation of the parent's window bounds to the submodules
    Noisiness()
    {
        this->addSubmodule(&totalAmp); // register the TotalAmplitude submodule
    }
    
    void doAnalysis(const float** input)
    {
        totalAmp.doAnalysis(input); // perform analysis with the TotalAmplitude submodule
        float total = totalAmp.getOutput(); // retrieve the output of the TotalAmplitude submodule
        
        float entropy = 0;
        // for each bin, calculate the contribution to the overall entropy
        for (int i = lowerBinBound; i < upperBinBound; i++) {
            // if the amplitude of the current bin is zero, skip it (log2(0) is undefined)
            if (input[CURR_WINDOW][i] > 0) {
                // assign a probability to each bin based on its amplitude
                float p = input[CURR_WINDOW][i] / total;
                // calculate the contribution of this bin to the entropy
                entropy -= p * log2(p);
            }
        }

        // Normalize the entropy normalized to a 0-1 scale
        output = entropy / log2(upperBinBound - lowerBinBound);
    }
};

#endif