#ifndef Mean_Amplitude_h
#define Mean_Amplitude_h

#include "../Analysis Module/AnalysisModule.h"
#include "TotalAmplitude.h"

class MeanAmplitude : public AnalysisModule<float>
{
private:
    TotalAmplitude totalAmp = TotalAmplitude();

public:
    MeanAmplitude()
    {
        submodules = &totalAmp;
    }
    
    void doAnalysis()
    {
        totalAmp.doAnalysis();
        float total = totalAmp.getOutput();
        output = total / (upperBinBound - lowerBinBound);
    }
};
#endif