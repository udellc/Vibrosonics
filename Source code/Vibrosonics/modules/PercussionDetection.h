#ifndef Percussion_Detection_h
#define Percussion_Detection_h

#include "../AnalysisModule.h"
#include "TotalAmplitude.h"
#include "Noisiness.h"

// TODO: enable delta analysis when input buffer is fixed

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