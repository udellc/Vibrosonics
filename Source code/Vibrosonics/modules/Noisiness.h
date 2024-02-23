#ifndef Noisiness_h
#define Noisiness_h

#include "../AnalysisModule.h"
#include "MaxAmplitude.h"
#include "MeanAmplitude.h"

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
        //Serial.printf("Lower %d, Upper %d, Noisiness: %f\n", lowerBinBound, upperBinBound, noisiness);
        output = noisiness;
    }
};

#endif