#include "Vibrosonics.h"
#include "Grain.h"
#include "CircularBuffer.h"
#include <arduinoFFTFloat.h>
#include <cmath>

// constants based on AudioLab definitions
float vReal[WINDOW_SIZE];
float vImag[WINDOW_SIZE];
arduinoFFT FFT = arduinoFFT();

const float frequencyResolution = float(SAMPLE_RATE) / WINDOW_SIZE;
const float frequencyWidth = 1.0 / frequencyResolution;

int* AudioLabInputBuffer = AudioLab.getInputBuffer();

void Vibrosonics::init() 
{
  Serial.begin(115200);
  while (!Serial)
    ;
  delay(4000);

  AudioLab.init();
}

void Vibrosonics::performFFT(int *input) {
  // copy samples from input to vReal and set vImag to 0
  for (int i = 0; i < WINDOW_SIZE; i++) {
    vReal[i] = input[i];
    vImag[i] = 0.0;
  }

  // use arduinoFFT
  FFT.DCRemoval(vReal, WINDOW_SIZE);                  // DC Removal to reduce noise
  FFT.Windowing(vReal, WINDOW_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
  FFT.Compute(vReal, vImag, WINDOW_SIZE, FFT_FORWARD);             // Compute FFT
  FFT.ComplexToMagnitude(vReal, vImag, WINDOW_SIZE);             // Compute frequency magnitudes
}

void Vibrosonics::storeFFTData(void) {
  // store pointer to freqsCurrent in freqsPrevious
  freqsPrevious = freqsCurrent;
  // increment rolling window counter [0..NUM_FREQ_WINDOWS)
  freqsWindowCounter += 1;
  if (freqsWindowCounter == NUM_FREQ_WINDOWS) freqsWindowCounter = 0;
  // set freqsCurrent pointer to the current freqs window
  freqsCurrent = freqs[freqsWindowCounter];

  // copy frequency magnitudes from vReal to freqsCurrent
  for (int i = 0; i < WINDOW_SIZE_BY2; i++) {
    // multiplying vReal by frequencyWidth is not necassary, but helps make frequency magnitudes
    // relative when using different window size and sample rate
    freqsCurrent[i] = vReal[i] * frequencyWidth;
  }
}

// writes FFT results to circular buffer, then updates submodule input pointers
void Vibrosonics::storeFFTDataCB() {
  buffer.write(vReal, frequencyWidth);
//   for(int i=0; i<numModules; i++){
//     modules[i]->setInputArrays(buffer.getPrevious(), buffer.getCurrent());
//   }
}

// finds and returns the mean amplitude
float Vibrosonics::getMean(float *data, int dataLength) {
  float _sum = 0.0;
  for (int i = 0; i < dataLength; i++) {
    _sum += data[i];
  }

  return _sum > 0.0 ? _sum / dataLength : _sum;
}

// sets amplitude of bin to 0 if less than threshold
void Vibrosonics::noiseFloor(float *data, float threshold) {
  for (int i = 0; i < WINDOW_SIZE_BY2; i++) {
    if (data[i] < threshold) {
      data[i] = 0.0;
    }
  }
}

// maps and normalizes amplitudes
void Vibrosonics::mapAmplitudes(float* ampData, int dataLength, float maxDataSum) {
  float _dataSum = 0.0;
  for (int i = 0; i < dataLength; i++) {
    _dataSum += ampData[i];
  }
  if (_dataSum == 0.0) return;
  float _divideBy = 1.0 / (_dataSum > maxDataSum ? _dataSum : maxDataSum);

  for (int i = 0; i < dataLength; i++) {
    ampData[i] = (ampData[i] * _divideBy);
  }
}

// creates and adds a single dynamic wave to a channel with the given freq and amp
void Vibrosonics::assignWave(float freq, float amp, int channel) {
  Wave _wave = AudioLab.dynamicWave(channel, freq, amp); 
}

// creates and adds dynamic waves for a list of frequencies and amplitudes
void Vibrosonics::assignWaves(float* freqData, float* ampData, int dataLength) {  
  int _bassIdx = 200 * frequencyWidth;
  // assign sin_waves and freq/amps that are above 0, otherwise skip
  for (int i = 0; i < dataLength; i++) {
    // skip storing if ampData is 0, or freqData is 0
    if (ampData[i] == 0.0 || freqData[i] == 0) continue;
    // assign frequencies below bass to left channel, otherwise to right channel
    float _interpFreq = 0;
    int _channel = 0;
    if (freqData[i] <= 100) {
      //float _freq = freqData[i];
      // if the difference of energy around the peak is greater than threshold
      //if (abs(freqsCurrent[freqData[i] - 1] - freqsCurrent[freqData[i] + 1]) > 100) {
      //  // assign frequency based on whichever side is greater
      //  _freq = freqsCurrent[freqData[i] - 1] > freqsCurrent[freqData[i] + 1] ? (freqData[i] - 0.5) : (freqData[i] + 0.5);
      //}
      //_interpFreq = round(_freq * frequencyResolution);
      _interpFreq = round(freqData[i]);
    } else {
      _channel = 1;
      _interpFreq = round(freqData[i]);
    }
    Wave _wave = AudioLab.dynamicWave(_channel, _interpFreq, ampData[i]);
    //Serial.printf("[Channel, freq, amp] = [%d, %g, %g]\n", _channel, _interpFreq, round(ampData[i]));
    //wave->set(_channel, _interpFreq, round(ampData[i]));
  }
}

// performs FFT on data stored in vReal, vImag then stores the results in circular buffer
void Vibrosonics::processInputCB()
{
  performFFT(AudioLabInputBuffer);
  storeFFTDataCB();

  return;
}

// runs doAnalysis on all added modules
void Vibrosonics::analyze()
{  
  // get data from circular buffer as 2D float array
  const float** data = buffer.unwind();
  
  for(int i=0; i<numModules; i++)
  {
    modules[i]->doAnalysis(data);
  }

  delete [] data;
}

// adds a new module to the Manager list of added modules
void Vibrosonics::addModule(AnalysisModule* module)
{  
    module->setWindowSize(WINDOW_SIZE);
    module->setSampleRate(SAMPLE_RATE);

  // create new larger array for modules
  numModules++;
  AnalysisModule** newModules = new AnalysisModule*[numModules];
  
  // copy modules over and add new module
  for(int i=0; i<numModules-1; i++){
    newModules[i] = modules[i];
  }
  newModules[numModules-1] = module;
  
  // free old modules array and store reference to new modules array
  delete [] modules;
  modules = newModules;
}

// maps frequencies from 0-SAMPLE_RATE to haptic range (0-250)
void Vibrosonics::mapFrequenciesLinear(float* freqData, int dataLength)
{
  float freqRatio;
  for (int i = 0; i < dataLength; i++) {
    freqRatio = freqData[i] / (SAMPLE_RATE>>1);
    freqData[i] = round(freqRatio * 250) + 20;
  }
}

// maps frequencies from 0-SAMPLE_RATE to haptic range (0-250Hz) using an exponent 
void Vibrosonics::mapFrequenciesExponential(float* freqData, int dataLength, float exp)
{
  float freqRatio;
  for (int i=0; i < dataLength; i++) {
    if (freqData[i] <= 50) { continue; }         // freq already within range
    freqRatio = freqData[i] / (SAMPLE_RATE>>1);
    freqData[i] = pow(freqRatio, exp) * 250;
  }
}