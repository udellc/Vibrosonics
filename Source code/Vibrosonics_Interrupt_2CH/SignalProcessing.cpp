#include "Vibrosonics.h"

// FFT data processing
void Vibrosonics::processData(void) {  
  // copy values calculated by FFT to freqs
  storeFreqs();

  // noise flooring based on a threshold
  noiseFloor(freqs, 20.0);

  // finds peaks in data, stores index of peak in FFTPeaks and Amplitude of peak in FFTPeaksAmp
  findMajorPeaks(freqs);

  // assign sine waves based on data found by major peaks
  resetWaves(0);
  resetWaves(1);
  
  assignWaves(FFTPeaks, FFTPeaksAmp, FFT_WINDOW_SIZE_BY2 >> 1);
  mapAmplitudes();
}


// store the current window into freqs
void Vibrosonics::storeFreqs (void) {
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    freqs[i] = vReal[i] * freqWidth;
  }
}

// noise flooring based on a set threshold
void Vibrosonics::noiseFloor(float *data, float threshold) {
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    if (data[i] < threshold) {
      data[i] = 0.0;
    }
  }
}

// finds all the peaks in the fft data* and removes the minimum peaks to contain output to @MAX_NUM_PEAKS
void Vibrosonics::findMajorPeaks(float* data) {
  // restore output arrays
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2 >> 1; i++) {
    FFTPeaks[i] = 0;
    FFTPeaksAmp[i] = 0;
  }
  // total sum of data
  int peaksFound = 0;
  float maxPeak = 0;
  int maxPeakIdx = 0;
  // iterate through data to find peaks
  for (int f = 1; f < FFT_WINDOW_SIZE_BY2 - 1; f++) {
    // determines if data[f] is a peak by comparing with previous and next location, otherwise continue
    if ((data[f - 1] < data[f]) && (data[f] > data[f + 1])) {
    float peakSum = data[f - 1] + data[f] + data[f + 1];
    if (peakSum > maxPeak) {
      maxPeak = peakSum;
      maxPeakIdx = f;
    }
    // store sum around the peak and index of peak
    FFTPeaksAmp[peaksFound] = peakSum;
    FFTPeaks[peaksFound++] = f;
    }
  }
  // if needed, remove a certain number of the minumum peaks to contain output to @maxNumberOfPeaks
  int numPeaksToRemove = peaksFound - MAX_NUM_PEAKS;
  for (int j = 0; j < numPeaksToRemove; j++) {
    // store minimum as the maximum peak
    float minimumPeak = maxPeak;
    int minimumPeakIdx = maxPeakIdx;
    // find the minimum peak and replace with zero
    for (int i = 0; i < peaksFound; i++) {
      float thisPeakAmplitude = FFTPeaksAmp[i];
      if (thisPeakAmplitude > 0 && thisPeakAmplitude < minimumPeak) {
        minimumPeak = thisPeakAmplitude;
        minimumPeakIdx = i;
      }
    }
    FFTPeaks[minimumPeakIdx] = 0;
    FFTPeaksAmp[minimumPeakIdx] = 0;
  }
}

// interpolation based on the weight of amplitudes around a peak
int Vibrosonics::interpolateAroundPeak(float *data, int indexOfPeak) {
  float prePeak = indexOfPeak == 0 ? 0.0 : data[indexOfPeak - 1];
  float atPeak = data[indexOfPeak];
  float postPeak = indexOfPeak == FFT_WINDOW_SIZE_BY2 ? 0.0 : data[indexOfPeak + 1];
  // summing around the index of maximum amplitude to normalize magnitudeOfChange
  float peakSum = prePeak + atPeak + postPeak;
  // interpolating the direction and magnitude of change, and normalizing from -1.0 to 1.0
  float magnitudeOfChange = ((atPeak + postPeak) - (atPeak + prePeak)) / (peakSum > 0.0 ? peakSum : 1.0);
  
  // return interpolated frequency
  return int(round((float(indexOfPeak) + magnitudeOfChange) * freqRes));
}

/*/
########################################################
Functions relating to assigning sine waves and mapping their frequencies and amplitudes
########################################################
/*/

// assigns the frequencies and amplitudes found by majorPeaks to sine waves
void Vibrosonics::assignWaves(int* freqData, float* ampData, int size) {  
  // assign sin_waves and freq/amps that are above 0, otherwise skip
  for (int i = 0; i < size; i++) {
    // skip storing if ampData is 0, or freqData is 0
    if (ampData[i] == 0.0 || freqData[i] == 0) continue;
    // assign frequencies below bass to left channel, otherwise to right channel
    if (freqData[i] <= bassIdx) {
      int freq = freqData[i];
      // if the difference of energy around the peak is greater than threshold
      if (abs(freqs[freqData[i] - 1] - freqs[freqData[i] + 1]) > 100) {
        // assign frequency based on whichever side is greater
        freq = freqs[freqData[i] - 1] > freqs[freqData[i] + 1] ? (freqData[i] - 0.5) : (freqData[i] + 0.5);
      }
      int id = addWave(freq * freqRes, ampData[i], 0, 0);
    } else {
      int interpFreq = interpolateAroundPeak(freqs, freqData[i]);
      #if NUM_OUT_CH == 2
      int id = addWave(interpFreq, ampData[i], 1, 0);
      #else
      int id = addWave(interpFreq, ampData[i], 0, 0);
      #endif
    }
  }
}