#ifndef Bread_Slicer_h
#define Bread_Slicer_h

#include "AnalysisModule.h"
#include <cmath>

// **Vibrosonics class will need freqWidth

/*  Analysis method that splits the frequency spectrum in to 'slices' then sums the
    amplitude within those ranges which are used as weights for a specified list
    of frequencies */
class BreadSlicer : public AnalysisModule<float*>
{
  public:
    int numFreqBands;
    int* frequencyBands;    // used for mapping frequencies to explicitly defined frequencies 
    float* breadSlicerSums;
    int* freqBandIndexes;
    int* breadSlicerPeaks;
    bool customParameters = false;  // track whether user has modified the object

    BreadSlicer()
    {
      numFreqBands = 5;  // default size
      frequencyBands = new int(numFreqBands);
      breadSlicerSums = new float(numFreqBands);
      freqBandIndexes = new int(numFreqBands);
      breadSlicerPeaks = new int(numFreqBands);

      frequencyBands = { 30, 48, 80, 100, 110 };  // default frequencies

      calculateFreqBandsIndexes();
    }
    ~BreadSlicer()
    {
      delete [] breadSlicerSums;
      delete [] freqBandIndexes;
      delete [] breadSlicerPeaks;
      if (customParameters) // only free if user did not pass in custom array
        delete [] frequencyBands;
    }

    void doAnalysis()
    {
      int sliceCount = 0;
      int sliceBinIdx = freqBandIndexes[sliceCount];
      float sliceSum = 0.0;
      int numBins = freqBandIndexes[sliceCount];
      int sliceMax = 0;
      int sliceMaxIdx = 0;
      for (int f = 0; f < windowSize>>1; f++) {
        // break if on last index of data array
        if (f == windowSize>>1 - 1) {
          breadSlicerSums[sliceCount] = (sliceSum + input[0][f]);
          breadSlicerPeaks[sliceCount] = sliceMaxIdx;
          break;
        }
        // get the sum and max amplitude of the current slice
        if (f < sliceBinIdx) {
          if (input[0][f] == 0) { continue; }
          sliceSum += input[0][f];
          if (f == 0) { continue; }
          // look for maximum peak in slice
          if ((input[0][f - 1] < input[0][f]) && (input[0][f] > input[0][f + 1])) {
            // compare with previous max amplitude at peak
            if (input[0][f] > sliceMax) {
              sliceMax = input[0][f];
              sliceMaxIdx = f;
            }
          }
          // if exceeding the index of this slice, store sliceSum and sliceMaxIdx
        } else {
          breadSlicerSums[sliceCount] = sliceSum;      // store slice sum
          breadSlicerPeaks[sliceCount] = sliceMaxIdx;  // store slice peak
          sliceSum = 0;                                // reset variables
          sliceMax = 0;
          sliceMaxIdx = 0;
          sliceCount += 1;  // iterate slice
          // break if sliceCount exceeds NUM_FREQ_BANDS
          if (sliceCount == numFreqBands) { break; }
          numBins = freqBandIndexes[sliceCount] - freqBandIndexes[sliceCount - 1];
          sliceBinIdx = freqBandIndexes[sliceCount];
          f -= 1;
        }
      }
      output = breadSlicerSums;
    }

    void changeNumFreqBands(int newSize, int* newFreqBands)
    {
      numFreqBands = newSize;

      delete [] frequencyBands;
      delete [] breadSlicerSums;
      delete [] freqBandIndexes;
      delete [] breadSlicerPeaks;

      //frequencyBands = new int(numFreqBands);
      breadSlicerSums = new float(numFreqBands);
      freqBandIndexes = new int(numFreqBands);
      breadSlicerPeaks = new int(numFreqBands);

      frequencyBands = newFreqBands;

      calculateFreqBandsIndexes();

      customParameters = true;
    }

    void calculateFreqBandsIndexes()
    {
      // converting from frequency to index
      for (int i = 0; i < numFreqBands; i++) {
        freqBandIndexes[i] = ceil(frequencyBands[i] * freqWidth);
      }
    }
};

#endif