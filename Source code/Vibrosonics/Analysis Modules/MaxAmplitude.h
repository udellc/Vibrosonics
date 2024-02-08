#ifndef Max_Amplitude_h
#define Max_Amplitude_h

#include "AnalysisModule.h"

class MaxAmplitude : public AnalysisModule<float>
{
public:
    void doAnalysis()
    {
        float max = 0.0;
        for(int i=0; i<windowSizeBy2; i++)
        {
            if(input[0][i] > max){ max = input[0][i]; }
        }
        output = max;
    }
};

#endif