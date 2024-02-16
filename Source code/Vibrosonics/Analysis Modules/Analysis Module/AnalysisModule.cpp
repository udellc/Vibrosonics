#include "AnalysisModule.h"

void AnalysisModule::addSubmodule(AnalysisModule *module)
{
    if(submodules)
    {
        int numSubmodules = sizeof(submodules)/sizeof(submodules[0]);
        AnalysisModule** newSubmodules = new AnalysisModule*[numSubmodules+1];
        for(int i=0; i<numSubmodules; i++)
        {
            newSubmodules[i] = submodules[i];
        }
        newSubmodules[numSubmodules] = module;
        delete[] submodules;
        submodules = newSubmodules;
    }
    else
    {
        submodules = new AnalysisModule*[1];
        submodules[0] = module;
    }
}

void AnalysisModule::setInputArrays(float* past, float* current)
{
    pastWindow = past;
    curWindow = current;

    if(submodules)
    {
        int numSubmodules = sizeof(submodules)/sizeof(submodules[0]);
        for(int i=0; i<numSubmodules; i++)
        {
            submodules[i]->setInputArrays(past, current);
        }
    }
}

void AnalysisModule::setAnalysisFreqRange(int lowerFreq, int upperFreq)
{
    if(lowerFreq < 0 || upperFreq > SAMPLE_RATE>>1 || lowerFreq > upperFreq)
    {
        Serial.println("Error: invalid frequency range");
        return;
    }

    // lower and upper are frequency values
    // convert frequencies to bin indices
    lowerBinBound = round(lowerFreq * freqWidth);
    upperBinBound = round(upperFreq * freqWidth);

    if(submodules)
    {
        int numSubmodules = sizeof(submodules)/sizeof(submodules[0]);
        for(int i=0; i<numSubmodules; i++)
        {
            submodules[i]->setAnalysisFreqRange(lowerFreq, upperFreq);
        }
    }
}