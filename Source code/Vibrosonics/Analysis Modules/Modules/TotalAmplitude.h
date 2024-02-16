#ifndef Total_Amplitude_h
#define Total_Amplitude_h

#include "../Analysis Module/AnalysisModule.h"

class TotalAmplitude : public ModuleInterface<float>
{
public:
    void doAnalysis()
    {
        float total = 0.0;
        for(int i=lowerBinBound; i<upperBinBound; i++)
        {
            total += curWindow[i];
        }
        output = total;
    }
};

#endif