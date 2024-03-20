//============================================================================
// MODULE INFORMATION
//============================================================================
// Name        : SalientFreqs
// Return Type : int* (array of freq. bin indexes with the highest amplitude delta)
// Description : SalientFreqs is used to find the n (numFreqs) bin indexes 
//               with the highest change in amplitude. The output is an array
//               of bin indexes, sorted by the magnitude of the change in
//               amplitude.
//============================================================================

#ifndef Salient_Freqs_h
#define Salient_Freqs_h

#include "../AnalysisModule.h"
#include "DeltaAmplitudes.h"

// SalientFreqs inherits from the ModuleInterface with an int* output type
class SalientFreqs : public ModuleInterface<int*>
{
  public:
    int numFreqs;
    int* salientFreqs;
    float* amplitudes;
    DeltaAmplitudes deltaAmps = DeltaAmplitudes();

    SalientFreqs()
    {
      int numFreqs = 3; // default to finding frequency of max change
      salientFreqs = new int[numFreqs];
      //amplitudes = new float[windowSize];
      this->addSubmodule(&deltaAmps);

      for (int i = 0; i < numFreqs; i++) {
        salientFreqs[i] = -1;
      }
    }
    ~SalientFreqs()
    {
      delete [] salientFreqs;
      //delete [] amplitudes;
    }

    // change the amount of salient frequencies to be found
    void changeNumFreqs(int newSize)
    {
      numFreqs = newSize;
      delete [] salientFreqs;
      salientFreqs = new int[numFreqs];
    }

    // finds the n (numFreqs) bins with highest change in amplitude, stored in salientFreqs[]
    // input is assumed to be a 2D array of FFT data passed in from the Vibrosonics class
    void doAnalysis(const float** input)
    {
      deltaAmps.doAnalysis(input);
      amplitudes = deltaAmps.getOutput();   // copy amplitudes

      // iterate through amplitudes to find the maximum(s)
      int currMaxAmp = 0; 
      int currMaxAmpIdx = -1;
      for (int i = 0; i < numFreqs+1; i++) {
        for (int j = lowerBinBound; j < upperBinBound; j++) {
          if (amplitudes[j] > currMaxAmp) {
            currMaxAmp = amplitudes[j];
            currMaxAmpIdx = j;
          }
        }
        Serial.printf("i: %d, ", i);
        Serial.printf("idx: %d, ", currMaxAmpIdx);
        salientFreqs[i] = currMaxAmpIdx;  // add the new max amp index to the array
        amplitudes[currMaxAmpIdx] = 0;  // set the amp to 0 so it will not be found again
        currMaxAmp = 0; // reset iterators
        currMaxAmpIdx = -1;
      } 
      output = salientFreqs;
      Serial.printf("SF: %d\n", output[0]);
    }

    void printSalientFreqs() {
        Serial.printf("SalientFreqs: ");
        for (int i = 0; i < numFreqs; i++) {
          Serial.printf("%d, ", salientFreqs[0]);
        }
        Serial.printf("\n");
    }

};

#endif