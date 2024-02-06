#include "Vibrosonics.h"

template <template T> class AnalysisModule
{
public:
    int windowSize;
    int** inputFreqs;
    T output;

    // IDEA: eventually add freq range constraints
    
    virtual void doAnalysis() = 0;
    T getOutput { return output; }
};

class MajorPeaks : public AnalysisModule<float**>
{
public:
    int numPeaks;
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

class TotalAmplitude : public AnalysisModule<float>
{

};

TotalAmplitude::doAnalysis()
{
  float total = 0.0;
  // add up total divide by bins
  output = total
}

class MeanAmplitude : public AnalysisModule<float>
{
  TotalAmplitude ta;
};

MeanAmplitude::doAnalysis()
{
  ta.doAnalysis();
  output = ta.output / windowSize>>1;
}

class Noisiness : public AnalysisModule<float>
{
  MeanAmplitude ma;
};

Noisiness::doAnalysis()
{
  ma.doAnalysis;
  float mean = ma.output;

  // do whatever

  output = //whatever
}

class PercussionDetection : public AnalysisModule<bool>{
  float volThreshold;
  float deltThreshold;
  float noiseThreshold;
  
  
  TotalAmplitude total;
  DeltaAmplitude delta;
  Noisiness noise;

  n.doAnalysis();
  n.getOutput();
  // high volume && big change in volume && high noisiness
}

PercussionDetection::doAnalysis()
{
  noise.doAnalysis();
  total.doAnalysis();
  delta.doAnalysis();

  if(noise.getOutput() > noiseThreshold &&
     total.getOutput() > volThreshold &&
     delta.getOutput() > deltThreshold)
  {
    output = 1;
  } else {
    output = 0;
  }
}