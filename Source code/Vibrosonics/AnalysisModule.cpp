#include "AnalysisModule.h"

void AnalysisModule::addSubmodule(AnalysisModule *module)
{
  
  // set module parameters
  // module->setInputArrays(pastWindow, curWindow);
  module->setAnalysisRangeByBin(lowerBinBound, upperBinBound);
  
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

// void AnalysisModule::setInputArrays(const float* past, const float* current)
// {
//   pastWindow = past;
//   curWindow = current;

//   for(int i=0; i<numSubmodules; i++)
//   {
//     // Serial.printf("setting input arrays for submodule %d\n", i);
//     submodules[i]->setInputArrays(past, current);
//   }
// }

void AnalysisModule::setAnalysisRangeByFreq(int lowerFreq, int upperFreq)
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
    submodules[i]->setAnalysisRangeByBin(lowerBinBound, upperBinBound);
  }
}

void AnalysisModule::setAnalysisRangeByBin(int lowerBin, int upperBin)
{
  if(lowerBin < 0 || upperBin > windowSize>>1 || lowerBin > upperBin)
  {
    Serial.println("Error: invalid frequency range");
    return;
  }

  // lower and upper are frequency values
  // convert frequencies to bin indices
  lowerBinBound = lowerBin;
  upperBinBound = upperBin;

  for(int i=0; i<numSubmodules; i++)
  {
    submodules[i]->setAnalysisRangeByBin(lowerBinBound, upperBinBound);
  }
}