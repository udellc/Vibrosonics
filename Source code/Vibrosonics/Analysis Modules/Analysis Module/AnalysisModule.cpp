#include "AnalysisModule.h"

template <typename T> void AnalysisModule<T>::setAnalysisFreqRange(int lower, int upper);
{
    if(lower < 0 || upper > SAMPLE_RATE>>1 || lower > upper)
    {
        Serial.println("Error: invalid frequency range");
        return;
    }

    // lower and upper are frequency values
    // convert frequencies to bin indices
    lowerBin = round(lower / freqWidth);
    upperBin = round(upper / freqWidth);

    if(submodules){
        for(int i=0; i<sizeof(submodules)/sizeof(submodules[0]); i++)
        {
            submodules[i]->setAnalysisFreqRange(lower, upper);
        }
    }
}