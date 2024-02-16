#include "AnalysisModule.h"


void AnalysisModule::addSubmodule(AnalysisModule *module)
{
  
  // set module parameters
  module->setInputArrays(pastWindow, curWindow);
  module->setAnalysisFreqRange(lowerBinBound * freqWidth, upperBinBound * freqWidth);
  
  // create new larger array for modules
  numSubmodules++;
  AnalysisModule** newSubmodules = new AnalysisModule*[numSubmodules];
  
  // copy modules over and add new module
  for(int i=0; i<numSubmodules-1; i++){
    newSubmodules[i] = submodules[i];
  }
  newSubmodules[numSubmodules-1] = module;
  
  // free old modules array and store reference to new modules array
  delete [] submodules;
  submodules = newSubmodules;
}

void AnalysisModule::setInputArrays(float* past, float* current)
{
  pastWindow = past;
  curWindow = current;

  for(int i=0; i<numSubmodules; i++)
  {
    submodules[i]->setInputArrays(past, current);
  }
}

void AnalysisModule::setAnalysisFreqRange(int lowerFreq, int upperFreq)
{
  if(lowerFreq < 0 || upperFreq > sampleRate>>1 || lowerFreq > upperFreq)
  {
    Serial.println("Error: invalid frequency range");
    return;
  }

  // lower and upper are frequency values
  // convert frequencies to bin indices
  lowerBinBound = round(lowerFreq * freqWidth);
  upperBinBound = round(upperFreq * freqWidth);

  for(int i=0; i<numSubmodules; i++)
  {
    submodules[i]->setAnalysisFreqRange(lowerFreq, upperFreq);
  }
}