#ifndef Noisiness_h
#define Noisiness_h

#include "AnalysisModule.h"
#include "MaxAmplitude.h"
#include "MeanAmplitude.h"

class Noisiness : public AnalysisModule<float>
{
private:
    MeanAmplitude mean = MeanAmplitude();
    MaxAmplitude max = MaxAmplitude();

public:
    void doAnalysis()
    {
        mean.doAnalysis();
        max.doAnalysis();
        
        float meanAmp = mean.getOutput();
        float maxAmp = max.getOutput();

        float noisiness = 0.0;
        if(meanAmp > 10.0){
            noisiness = 1 - ((maxAmp / meanAmp) / 66.7);
        }
        output = noisiness;
    }
};

#endif