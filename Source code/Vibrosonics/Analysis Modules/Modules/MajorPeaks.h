#include "../Analysis Module/MajorPeaks.h"

class MajorPeaks : public AnalysisModule<float**>
{

};

void MajorPeaks::doAnalysis()
{
  // restore output arrays
  for (int i = 0; i < WINDOW_SIZE_BY2 >> 1; i++) {
    outputFrequencies[i] = 0;
    outputAmplitudes[i] = 0;
  }
  // total sum of data
  int _peaksFound = 0;
  float _maxPeak = 0;
  int _maxPeakIdx = 0;
  // iterate through data to find peaks
  for (int f = 1; f < WINDOW_SIZE_BY2 - 1; f++) {
    // determines if data[f] is a peak by comparing with previous and next location, otherwise continue
    if ((inputFreqs[0][f - 1] < data[f]) && (data[f] > data[f + 1])) {
    float _peakSum = data[f - 1] + data[f] + data[f + 1];
    if (_peakSum > _maxPeak) {
      _maxPeak = _peakSum;
      _maxPeakIdx = f;
    }
    // store sum around the peak and index of peak
    outputAmplitudes[_peaksFound] = _peakSum;
    outputFrequencies[_peaksFound++] = f;
    }
  }
  // if needed, remove a certain number of the minumum peaks to contain output to @maxNumPeaks
  int _numPeaksToRemove = _peaksFound - maxNumPeaks;
  for (int j = 0; j < _numPeaksToRemove; j++) {
    // store minimum as the maximum peak
    float _minimumPeak = _maxPeak;
    int _minimumPeakIdx = _maxPeakIdx;
    // find the minimum peak and replace with zero
    for (int i = 0; i < _peaksFound; i++) {
      float _thisPeakAmplitude = FFTPeaksAmp[i];
      if (_thisPeakAmplitude > 0 && _thisPeakAmplitude < _minimumPeak) {
        _minimumPeak = _thisPeakAmplitude;
        _minimumPeakIdx = i;
      }
    }
    outputFrequencies[_minimumPeakIdx] = 0;
    outputAmplitudes[_minimumPeakIdx] = 0;
  }
  output[0] = outputFrequencies;
  output[1] = outputAmplitudes;
}
