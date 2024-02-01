#include "Vibrosonics.h"


/******************************************************************************
**  GENERATOR BASE CLASS
******************************************************************************/
class Generator
{
    public:
        // public member functions
        virtual void generate() = 0;
        void addInputSource(AnalysisModule* aMod);
        void setWindowSize();
        void addOutputChannel(int channelNum)
        void rmvOutputChannel(int channelNum)
        
        // constructors
        Generator();
        Generator(AnalysisModule* aMod);
    protected:
        // protected data
        AnalysisModule* inputSource;
        int windowSize;
        int outputChannels[2];
};   

{
    AdditiveSynth s;
    s.addOutputChannel(0);
    s.addOutputChannel(1);

    s.rmvOutputChannel(0);

    int mychannels[5] = {0, 1, 5, 6, 7};
    s.addOutputChannels(mychannels);
}

// default constructor
Generator::Generator()
{
    windowSize = 256;   // default window size
}

// constructor with user provided input source
Generator::Generator(AnalysisModule* aMod)
{
    windowSize = 256;   // default window size
    inputSources = aMod; // input source
}

Generator::setWindowSize(int newSize)
{
    windowSize = newSize;
    // set inputSources window sizes
}

Generator::addInputSource(AnalysisModule* aMod)
{
    aMod.setWindowSize(windowSize); // set window size of input source
    // add module to inputSources   
}

Generator::addOutputChannel(int channelNum)
{

}

Generator::rmvOutputChannel(int channelNum)
{

}

/******************************************************************************
**  ADDITIVE SYNTHESIZER
**  Desc: This object is derived from the base generator and is used to generate
**  sustained waves from a list of frequencies and amplitudes.
******************************************************************************/
class AdditiveSynth : public Generator
{
public:
    // member functions
    mapAmplitudes(float* ampData, int dataLength, float maxDataSum);
    assignWaves(int* freqData, float* ampData, int dataLength);
    interpolateAroundPeak(float *data, int indexOfPeak);

    // constructors
    AdditiveSynth() : Generator();
    AdditiveSynth(AnalysisModule* aMod) : Generator(aMod);
};

AdditiveSynth::generate()
{    
    mapAmplitudes(inputSource->outputAmplitudes, inputSource->outputLength, 3000);
    assignWaves(inputSourc->outputFrequencies, inputSource->outputAmplitudes, inputSource->outputLength);
    //AudioLab.synthesize(); ???
}

AdditiveSynth::mapAmplitudes(float* ampData, int dataLength, float maxDataSum) {
  float _dataSum = 0.0;
  for (int i = 0; i < dataLength; i++) {
    _dataSum += ampData[i];
  }
  if (_dataSum == 0.0) return;
  float _divideBy = 1.0 / (_dataSum > maxDataSum ? _dataSum : maxDataSum);

  for (int i = 0; i < dataLength; i++) {
    ampData[i] = round(ampData[i] * _divideBy * 127.0);
  }
}

AdditiveSynth::assignWaves(int* freqData, float* ampData, int dataLength) {  
  int _bassIdx = 200 * frequencyWidth;
  // assign sin_waves and freq/amps that are above 0, otherwise skip
  for (int i = 0; i < dataLength; i++) {
    // skip storing if ampData is 0, or freqData is 0
    if (ampData[i] == 0.0 || freqData[i] == 0) continue;
    // assign frequencies below bass to left channel, otherwise to right channel
    int _interpFreq = 0;
    int _channel = 0;
    if (freqData[i] <= _bassIdx) {
      float _freq = freqData[i];
      // if the difference of energy around the peak is greater than threshold
      if (abs(freqsCurrent[freqData[i] - 1] - freqsCurrent[freqData[i] + 1]) > 100) {
        // assign frequency based on whichever side is greater
        _freq = freqsCurrent[freqData[i] - 1] > freqsCurrent[freqData[i] + 1] ? (freqData[i] - 0.5) : (freqData[i] + 0.5);
      }
      _interpFreq = round(_freq * frequencyResolution);
    } else {
      _channel = 1;
      _interpFreq = interpolateAroundPeak(freqsCurrent, freqData[i]);
    }
    Wave _wave = AudioLab.dynamicWave(_channel, _interpFreq, round(ampData[i]));
    //wave->set(_channel, _interpFreq, round(ampData[i]));
  }
}

// interpolation based on the weight of amplitudes around a peak
AdditiveSynth::interpolateAroundPeak(float *data, int indexOfPeak) {
  float prePeak = indexOfPeak == 0 ? 0.0 : data[indexOfPeak - 1];
  float atPeak = data[indexOfPeak];
  float postPeak = indexOfPeak == WINDOW_SIZE_BY2 ? 0.0 : data[indexOfPeak + 1];
  // summing around the index of maximum amplitude to normalize magnitudeOfChange
  float peakSum = prePeak + atPeak + postPeak;
  // interpolating the direction and magnitude of change, and normalizing from -1.0 to 1.0
  float magnitudeOfChange = ((atPeak + postPeak) - (atPeak + prePeak)) / (peakSum > 0.0 ? peakSum : 1.0);
  
  // return interpolated frequency
  return int(round((float(indexOfPeak) + magnitudeOfChange) * frequencyResolution));
}

/******************************************************************************
**  PULSE GENERATOR
**  Desc: This object is derived from the base generator and is used to generate
**  pulses.
******************************************************************************/
class PulseGenerator : public Generator
{
public:
    Pulse p;
};