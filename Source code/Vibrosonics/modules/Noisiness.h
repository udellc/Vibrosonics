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

// Noisiness inherits from the ModuleInterface with a float output type
class Noisiness : public ModuleInterface<float>
{
private:
    MeanAmplitude mean = MeanAmplitude();
    MaxAmplitude max = MaxAmplitude();

public:
    Noisiness()
    {
        this->addSubmodule(&mean);
        this->addSubmodule(&max);
    }
    
    void doAnalysis()
    {
        mean.doAnalysis();
        max.doAnalysis();
        
        float meanAmp = mean.getOutput();
        float maxAmp = max.getOutput();

        float noisiness = 0.0;
        if(meanAmp > 1.0){
            noisiness = 1 - ((maxAmp / meanAmp) / 66.7);
        }
        output = noisiness;
    }
};

#endif