//============================================================================
// MODULE INFORMATION
//============================================================================
// Name        : BreadSlicer
// Return Type : float*
// Description : Analysis method that splits the frequency spectrum into slices,
//               sums the amplitude within those ranges, and uses the sums as 
//               weights for a specified list of output frequencies. 
//============================================================================
#ifndef Bread_Slicer_h
#define Bread_Slicer_h

#include "../AnalysisModule.h"
#include <cmath>

// BreadSlicer inherits from the ModuleInterface with a float* output type
class BreadSlicer : public ModuleInterface<float*>
{
  private:
    int *bandIndexes;
    int numBands;

  public:
    // default constructor, initializes private members.
    // For now this is the only constructor so setBands() must be used to setup this module
    BreadSlicer() {
      this->numBands = 0;       // initialize number of bands to 0

      this->bandIndexes = NULL; // initialize index array to null
      this->output = NULL;      // initialize output pointer to null
    }
    // deconstructor, frees member pointers if memory was allocated
    ~BreadSlicer() {
      if (this->bandIndexes != NULL) free(bandIndexes);
      if (this->output != NULL) free(output);
    }

    /* sets the bands ('slices') of this module
      Ex. setBands([0, 200, 500, 2000, 4000], 4);
      Band frequencies must be in ascending order, frequencies must be at least 
      freqResolution-Hz apart so bands dont overlap
    */
    void setBands(int *frequencyBands, int numBands) {
      int _nyquist = sampleRate >> 1; // nyquist frequency is 1/2 the sampleRate

      for (int i = 0; i < numBands; i++) {  // check if each band is greater than the previous and less than next
        if (!((frequencyBands[i] >= 0 && frequencyBands[i] <= _nyquist) &&
            (frequencyBands[i] < frequencyBands[i + 1] && frequencyBands[i + 1] <= _nyquist)))
        {
          Serial.println("BreadSlicer setBands() fail! Invalid bands!");
          return;
        }
      }

      // free old bandIndexes and output pointers if bands have already been set
      if (this->bandIndexes != NULL) free(bandIndexes);   // free bandIndexes
      if (this->output != NULL) free(output);             // free output

      // allocate memory for new bandIndexes and outpt pointers
      this->bandIndexes = (int*)malloc(sizeof(int) * numBands + 1);
      this->output = (float*)malloc(sizeof(float) * numBands);
      this->numBands = numBands;                          // set new number of bands

      // find the FFT bin index of each freq and store it in bandIndexes
      for (int i = 0; i < numBands + 1; i++) {
        bandIndexes[i] = round(frequencyBands[i] * freqWidth);
        if (i < numBands) {
          this->output[i] = 0.0;  // initialize amplitude of slice to 0
        }
      }
    }
    
    // Sums the amplitudes in each frequency band and stores the results in output.
    void doAnalysis(const float** input) {
      if (this->bandIndexes == NULL) return;  // do not run analysis if bands are not set

      int _bandIndex = 1;
      float _bandSum = 0;
      // finds the total amplitude of each band by summing the bins within each band
      // then stores that value in output
      for (int i = this->bandIndexes[0]; i <= this->bandIndexes[this->numBands]; i++) {
        if (i < this->bandIndexes[_bandIndex]) {
          _bandSum += input[CURR_WINDOW][i];
        }
        else {
        this->output[_bandIndex - 1] = _bandSum;
        _bandIndex += 1;
        _bandSum = 0;
        i -= 1;
        }
      }
    }
};

#endif