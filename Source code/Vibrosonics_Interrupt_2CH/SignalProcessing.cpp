#include "Vibrosonics.h"

// FFT data processing
void Vibrosonics::processData(void) {  
  resetAllWaves();

  // copy values calculated by FFT to freqs
  storeFreqs();

  // noise flooring based on a threshold
  // if (getMean(freqsCurrent, FFT_WINDOW_SIZE_BY2) < 1.0) {
  //   return;
  // }

  noiseFloor(freqsCurrent, 20.0);
  
  // find frequency of most change and its magnitude between the current and previous window
  #ifndef MIRROR_MODE
  float freqOfMaxChangeMag = 0.0;
  int freqOfMaxChange = frequencyMaxAmplitudeDelta(freqsCurrent, freqsPrevious, 200, 4000, freqOfMaxChangeMag);

  // add wave for frequency of max amplitude change
  if (freqOfMaxChange > 0) {
    int amplitude = sumOfPeak(freqsCurrent, freqOfMaxChange) + sumOfPeak(freqsPrevious, freqOfMaxChange);
    int frequency = interpolateAroundPeak(freqsCurrent, freqOfMaxChange);
    frequency = freqWeighting(frequency);
    #if AUD_OUT_CH == 1
    addWave(0, frequency, amplitude * freqOfMaxChangeMag);
    #else
    addWave(1, frequency, amplitude * freqOfMaxChangeMag);
    #endif
  }
  #endif

  // finds peaks in data, stores index of peak in FFTPeaks and Amplitude of peak in FFTPeaksAmp
  findMajorPeaks(freqsCurrent);

  // assign sine waves based on data found by major peaks
  assignWaves(FFTPeaks, FFTPeaksAmp, FFT_WINDOW_SIZE_BY2 >> 1);
  mapAmplitudes();
  addWave(0, 50, 127);
}


void Vibrosonics::storeFreqs(void) {
  freqsWindowPrev = freqsWindow;
  freqsWindow += 1;
  if (freqsWindow >= NUM_FREQ_WINDOWS) freqsWindow = 0;
  freqsPrevious = freqs[freqsWindowPrev];
  freqsCurrent = freqs[freqsWindow];
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    freqsCurrent[i] = vReal[i] * freqWidth;
    // Serial.println(vReal[i]);
  }
}

void Vibrosonics::noiseFloor(float *data, float threshold) {
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    if (data[i] < threshold) {
      data[i] = 0.0;
    }
  }
}

float Vibrosonics::getMean(float *data, int size) {
  float sum = 0.0;
  for (int i = 0; i < size; i++) {
    sum += data[i];
  }
  return sum > 0.0 ? sum / size : 0.0;
}

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

int Vibrosonics::frequencyMaxAmplitudeDelta(float *data, float *prevData, int minFreq, int maxFreq, float &magnitude) {
  // calculate indexes in FFT bins correspoding to minFreq and maxFreq
  int minIdx = round(minFreq * freqWidth);
  if (minIdx == 0) minIdx = 1;
  int maxIdx = round(maxFreq * freqWidth);
  if (maxIdx > FFT_WINDOW_SIZE_BY2 - 1) maxIdx = FFT_WINDOW_SIZE_BY2 - 1;
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

int Vibrosonics::sumOfPeak(float *data, int indexOfPeak) {
  if (indexOfPeak > 0 && indexOfPeak < FFT_WINDOW_SIZE_BY2 - 1) {
    return data[indexOfPeak - 1] + data[indexOfPeak] + data[indexOfPeak + 1];
  }
  return 0;
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
    Serial.println(freqData[i]);
    // assign frequencies below bass to left channel, otherwise to right channel
    if (freqData[i] <= bassIdx) {
      int freq = freqData[i];
      // if the difference of energy around the peak is greater than threshold
      if (abs(freqsCurrent[freqData[i] - 1] - freqsCurrent[freqData[i] + 1]) > 100) {
        // assign frequency based on whichever side is greater
        freq = freqsCurrent[freqData[i] - 1] > freqsCurrent[freqData[i] + 1] ? (freqData[i] - 0.5) : (freqData[i] + 0.5);
      }
      addWave(freq * freqRes, ampData[i], 0, 0);
    } else {
      int interpFreq = interpolateAroundPeak(freqsCurrent, freqData[i]);
      Serial.println(interpFreq);

      #ifndef MIRROR_MODE
      interpFreq = freqWeighting(interpFreq);
      #endif

      #if NUM_OUT_CH == 1
      addWave(0, interpFreq, ampData[i]);
      #else
      addWave(1, interpFreq, ampData[i]);
      #endif
    }
  }
}

int Vibrosonics::freqWeighting(int freq) {
  // normalize between 0 and 1.0, where 0.0 corresponds to 120Hz, while SAMPLING_FREQ / 2 Hz corresponds to 1.0
  float freq_n = (freq - 120) / float(SAMPLING_FREQ * 0.5 - 120);
  // broaden range for lower frequencies
  freq_n = pow(freq_n, 0.2);
  // multiply by maximum desired value
  freq = round(freq_n * 120);
}
