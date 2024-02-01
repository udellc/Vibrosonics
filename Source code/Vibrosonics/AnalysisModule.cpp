#include "Vibrosonics.h"

class AnalysisModule
{
public:
    int windowSize;
    int outputLength;
    int* outputFrequencies;
    float* outputAmplitudes;
    virtual void doAnalysis() = 0;

    // constructors
    AnalysisModule();
    AnalysisModule(int n)
};

AnalysisModule::AnalysisModule()
{
    outputLength = 0;
    // allocate default output length
    // outputFrequencies = new int(0);
    // outputAmplitude = new float(0);
}

AnalysisModule::AnalysisModule(int n)
{
    outputLength = n;
    // allocate provided output length
    outputFrequencies = new int(outputLength);
    outputAmplitudes = new float(outputLength);
}

AnalysisModule::~AnalysisModule()
{
    delete outputFrequencies;
    delete outputAmplitudes;
}

class MajorPeaks : public AnalysisModule
{
public:
    int numPeaks;
    
    // constructors
    MajorPeaks() : AnalysisModule(16);
    MajorPeaks(int n) : AnalysisModule(int n);
};

{
    Vibrosonics v = Vibrosonics(); // create vibrosonics obj

    // add synthesizer to channel 0
    AdditiveSynth synth = AdditiveSynth(); // create generator
    v.addGenerator(synth, channel_0);      // connect generator

    // connect synth to majorpeaks analysis module
    MajorPeaks m1 = MajorPeaks(); // create analysis module
    synth.addInputSource(m1);     // connect analysis module

    // add pulse generator to channel 1
    PulseGenerator p = PulseGenerator(); // create generator
    v.addGenerator(p, channel_1);        // connect generator
    
    // connect percussion detection to pulse generator
    PercussionDetection det = PercussionDetection(); // create analysis module
    p.addInputSource(det);                           // connect analysis module
}

{
     Vibrosonics v = Vibrosonics(); // create vibrosonics obj

    MajorPeaks m1 = MajorPeaks(); // create analysis module
    // add synthesizer to channel 0
    AdditiveSynth synth = AdditiveSynth(m1); // create generator
    v.addGenerator(synth, channel_0);      // connect generator

    // add pulse generator to channel 1
    PulseGenerator p = PulseGenerator(); // create generator
    v.addGenerator(p, channel_1);        // connect generator
    
    // connect percussion detection to pulse generator
    PercussionDetection det = PercussionDetection(); // create analysis module
    p.addInputSource(det);                           // connect analysis module
}

MajorPeaks::MajorPeaks()
{
    numPeaks = 8; // default value
}

MajorPeaks::MajorPeaks(int n)
{
    numPeaks = n; // user provided value
}

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
    if ((data[f - 1] < data[f]) && (data[f] > data[f + 1])) {
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
}