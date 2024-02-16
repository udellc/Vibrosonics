#ifndef Max_Amplitude_h
#define Max_Amplitude_h

#include "../Analysis Module/AnalysisModule.h"

class MaxAmplitude : public AnalysisModule<float>
{
public:
    void doAnalysis()
    {
        float max = 0.0;
        for(int i=lowerBinBound; i<upperBinBound; i++)
        {
            if(curWindow[i] > max){ max = curWindow[i]; }
        }
        output = max;
    }
};

#endif