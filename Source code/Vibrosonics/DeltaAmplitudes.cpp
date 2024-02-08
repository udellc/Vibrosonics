#include "Vibrosonics.h"
#include "AnalysisModule.h"

/*  Used to find the change in amplitudes between the current and previous FFT window
    for each bin */ 
class DeltaAmplitudes:: public AnalysisModule<int*>
{
  public:
    int* deltaAmplitudes; 

    DeltaAmplitudes(); 
}

DeltaAmplitudes::DeltaAmplitudes()
{
  deltaAmplitudes = new int[windowSize];
}

DeltaAmplitudes::~DeltaAmplitudes()
{
  delete [] deltaAmplitudes;
}

void DeltaAmplitudes::doAnalysis()
{
  // iterate through FFT data and store the change in amplitudes between current and previous window
  int ampChange = 0;
  for (int i = 0; i < windowSize>>1; i++) {
    deltaAmplitudes[i] = abs(int(inputFreqs[0][i]) - int(inputFreqs[1][i])) 
  }

  output = deltaAmplitudes;
}