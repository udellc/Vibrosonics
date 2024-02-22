#ifndef Mean_Amplitude_h
#define Mean_Amplitude_h

#include "../AnalysisModule.h"
#include "TotalAmplitude.h"

class MeanAmplitude : public ModuleInterface<float>
{
private:
    TotalAmplitude totalAmp = TotalAmplitude();

public:
    MeanAmplitude()
    {
        addSubmodule(&totalAmp);
    }
    
    void doAnalysis()
    {
        totalAmp.doAnalysis();
        float total = totalAmp.getOutput();
        output = total / (upperBinBound - lowerBinBound);
    }
};
#endif