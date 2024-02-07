#ifndef Total_Amplitude_h
#define Total_Amplitude_h

#include "AnalysisModule.h"

class TotalAmplitude : public AnalysisModule<float>
{
public:
    void doAnalysis()
    {
        float total = 0.0;
        for(int i=0; i<windowSizeBy2; i++)
        {
            total += input[0][i];
        }
        output = total;
    }
};

#endif