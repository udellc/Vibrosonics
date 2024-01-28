#include "Vibrosonics.h"
#include <arduinoFFTFloat.h>

float vReal[WINDOW_SIZE];
float vImag[WINDOW_SIZE];
arduinoFFT FFT = arduinoFFT();

const float frequencyResolution = float(SAMPLE_RATE) / WINDOW_SIZE;
const float frequencyWidth = 1.0 / frequencyResolution;

int Vibrosonics::FFTMajorPeak(int sampleRate) {
  int maxIdx = 0;
  int max = 0;
  for (int i = 0; i < WINDOW_SIZE_BY2; i++) {
    if (vReal[i] > max) {
      max = vReal[i];
      maxIdx = i;
    }
  }
  return maxIdx * sampleRate / WINDOW_SIZE;
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

void Vibrosonics::storeFFTDataLow(void) {
  for (int i = 0; i < WINDOW_SIZE_BY2; i++) {
    freqsLow[i] = vReal[i];
  }
}

float Vibrosonics::getMean(float *data, int dataLength) {
  float _sum = 0.0;
  for (int i = 0; i < dataLength; i++) {
    _sum += data[i];
  }

  return _sum > 0.0 ? _sum / dataLength : _sum;
}

void Vibrosonics::noiseFloor(float *data, float threshold) {
  for (int i = 0; i < WINDOW_SIZE_BY2; i++) {
    if (data[i] < threshold) {
      data[i] = 0.0;
    }
  }
}

void Vibrosonics::findPeaks(float* data, int maxNumPeaks) {
  // restore output arrays
  for (int i = 0; i < WINDOW_SIZE_BY2 >> 1; i++) {
    FFTPeaks[i] = 0;
    FFTPeaksAmp[i] = 0;
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
    FFTPeaksAmp[_peaksFound] = _peakSum;
    FFTPeaks[_peaksFound++] = f;
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
    FFTPeaks[_minimumPeakIdx] = 0;
    FFTPeaksAmp[_minimumPeakIdx] = 0;
  }
}

void Vibrosonics::mapAmplitudes(float* ampData, int dataLength, float maxDataSum) {
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

void Vibrosonics::assignWaves(int* freqData, float* ampData, int dataLength) {  
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

int Vibrosonics::interpolateAroundPeak(float *data, int indexOfPeak, int sampleRate, int windowSize) {
  float _freqRes = sampleRate * 1.0 / windowSize;
  float prePeak = indexOfPeak == 0 ? 0.0 : data[indexOfPeak - 1];
  float atPeak = data[indexOfPeak];
  float postPeak = indexOfPeak == WINDOW_SIZE_BY2 ? 0.0 : data[indexOfPeak + 1];
  // summing around the index of maximum amplitude to normalize magnitudeOfChange
  float peakSum = prePeak + atPeak + postPeak;
  // interpolating the direction and magnitude of change, and normalizing from -1.0 to 1.0
  float magnitudeOfChange = ((atPeak + postPeak) - (atPeak + prePeak)) / (peakSum > 0.0 ? peakSum : 1.0);
  
  // return interpolated frequency
  return int(round((float(indexOfPeak) + magnitudeOfChange) * _freqRes));
}

int Vibrosonics::sumOfPeak(float *data, int indexOfPeak) {
  if (indexOfPeak > 0 && indexOfPeak < WINDOW_SIZE_BY2 - 1) {
    return data[indexOfPeak - 1] + data[indexOfPeak] + data[indexOfPeak + 1];
  }
  return 0;
}

int Vibrosonics::frequencyMaxAmplitudeDelta(float *data, float *prevData, int minFreq, int maxFreq, float &magnitude) {
  // calculate indexes in FFT bins correspoding to minFreq and maxFreq
  int minIdx = round(minFreq * frequencyWidth);
  if (minIdx == 0) minIdx = 1;
  int maxIdx = round(maxFreq * frequencyWidth);
  if (maxIdx > WINDOW_SIZE_BY2 - 1) maxIdx = WINDOW_SIZE_BY2 - 1;
  // restore global varialbes
  int maxAmpChange = 0;
  int maxAmpChangeIdx = -1;
  // iterate through data* and prevData* to find the amplitude with most change
  for (int i = minIdx; i < maxIdx; i++) {
    // calculate the change between this amplitude and previous amplitude
    int sumAroundDataI = sumOfPeak(data, i);
    int sumAroundPrevDataI = sumOfPeak(prevData, i);
    int currAmpChange = abs(sumAroundDataI - sumAroundPrevDataI);
    // find the most dominant amplitude change and store in maxAmpChangeIdx, store the magnitude of the change in maxAmpChange
    if ((currAmpChange >= FREQ_MAX_AMP_DELTA_MIN) && (currAmpChange <= FREQ_MAX_AMP_DELTA_MAX) && (currAmpChange > maxAmpChange)) {
      maxAmpChange = currAmpChange;
      maxAmpChangeIdx = i;
    }
  }
  magnitude = maxAmpChange * (1.0 / FREQ_MAX_AMP_DELTA_MAX) * FREQ_MAX_AMP_DELTA_K;
  return maxAmpChangeIdx;
}