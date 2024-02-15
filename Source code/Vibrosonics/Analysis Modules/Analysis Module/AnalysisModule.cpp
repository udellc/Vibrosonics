#include "AnalysisModule.h"

template <typename T> void AnalysisModule<T>::setAnalysisFreqRange(int lowerFreq, int upperFreq);
{
    if(lowerFreq < 0 || upperFreq > SAMPLE_RATE>>1 || lowerFreq > upperFreq)
    {
        Serial.println("Error: invalid frequency range");
        return;
    }

    // lower and upper are frequency values
    // convert frequencies to bin indices
    lowerBin = round(lowerFreq * freqWidth);
    upperBin = round(upperFreq * freqWidth);

    if(submodules)
    {
        for(int i=0; i<sizeof(submodules)/sizeof(submodules[0]); i++)
        {
            submodules[i]->setAnalysisFreqRange(lower, upper);
        }
    }
}