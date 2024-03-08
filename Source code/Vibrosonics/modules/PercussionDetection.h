//============================================================================
// MODULE INFORMATION
//============================================================================
// Name        : PercussionDetection
// Return Type : bool (percussion detected in the current window)
// Description : This module is used to detect the presence of noisy 
//               transients. It uses the TotalAmplitude, DeltaAmplitude, and 
//               Noisiness modules to determine if a percussive sound is 
//               present. The thresholds for each of these modules can be set 
//               by the user to better fit the specific qualities of the input 
//               signal.
//============================================================================

#ifndef Percussion_Detection_h
#define Percussion_Detection_h

#include "../AnalysisModule.h"
#include "TotalAmplitude.h"
#include "Noisiness.h"

// TODO: enable delta analysis when input buffer is fixed

// PercussionDetection inherits from the ModuleInterface with a bool output type
class PercussionDetection : public ModuleInterface<bool>
{
private:
    TotalAmplitude totalAmp = TotalAmplitude();
    //DeltaAmplitude deltaAmp = DeltaAmplitude();
    Noisiness noise = Noisiness();

    float loudness_threshold = 60.0;
    //float delta_threshold = 0.8 * loudness_threshold;
    float noise_threshold = 0.85;

public:
    PercussionDetection()
    {
        this->addSubmodule(&totalAmp);
        //this->addSubmodule(&deltaAmp);
        this->addSubmodule(&noise);
    }
    
    void doAnalysis()
    {
        totalAmp.doAnalysis();
        //deltaAmp.doAnalysis();
        noise.doAnalysis();

        float total = totalAmp.getOutput();
        //float delta = deltaAmp.getOutput();
        
        float noiseOutput = noise.getOutput();

        output = (total >= loudness_threshold) /*&& (delta >= delta_threshold) */&& (noiseOutput >= noise_threshold);
    }
};
#endif