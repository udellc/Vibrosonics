#ifndef Bread_Slicer_h
#define Bread_Slicer_h

#include "../AnalysisModule.h"
#include <cmath>

// **Vibrosonics class will need freqWidth

/*  Analysis method that splits the frequency spectrum in to 'slices' then sums the
    amplitude within those ranges which are used as weights for a specified list
    of frequencies */
class BreadSlicer : public ModuleInterface<float*>
{
  public:
    int numFreqBands;
    float* breadSlicerSums;
    int* freqBandIndexes;
    int* breadSlicerPeaks;

    BreadSlicer()
    {
      numFreqBands = 5;  // default size
      breadSlicerSums = new float(numFreqBands);
      freqBandIndexes = new int(numFreqBands);
      breadSlicerPeaks = new int(numFreqBands);

      calculateFreqBandsIndexes();
    }
    ~BreadSlicer()
    {
      delete [] breadSlicerSums;
      delete [] freqBandIndexes;
      delete [] breadSlicerPeaks;
    }

    void doAnalysis()
    {
      int sliceCount = 0;
      int sliceBinIdx = freqBandIndexes[sliceCount];
      float sliceSum = 0.0;
      int numBins = freqBandIndexes[sliceCount];
      int sliceMax = 0;
      int sliceMaxIdx = 0;
      for (int f = lowerBinBound; f < upperBinBound; f++) {
        // break if on last index of data array
        if (f == windowSize>>1 - 1) {
          breadSlicerSums[sliceCount] = (sliceSum + curWindow[f]);
          breadSlicerPeaks[sliceCount] = sliceMaxIdx;
          break;
        }
        // get the sum and max amplitude of the current slice
        if (f < sliceBinIdx) {
          if (curWindow[f] == 0) { continue; }
          sliceSum += curWindow[f];
          if (f == 0) { continue; }
          // look for maximum peak in slice
          if ((curWindow[f - 1] < curWindow[f]) && (curWindow[f] > curWindow[f + 1])) {
            // compare with previous max amplitude at peak
            if (curWindow[f] > sliceMax) {
              sliceMax = curWindow[f];
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

    void changeNumFreqBands(int newSize)
    {
      numFreqBands = newSize;

      delete [] breadSlicerSums;
      delete [] freqBandIndexes;
      delete [] breadSlicerPeaks;

      //frequencyBands = new int(numFreqBands);
      breadSlicerSums = new float(numFreqBands);
      freqBandIndexes = new int(numFreqBands);
      breadSlicerPeaks = new int(numFreqBands);

      calculateFreqBandsIndexes();
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