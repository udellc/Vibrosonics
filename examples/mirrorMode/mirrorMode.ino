#include "VibrosonicsAPI.h"

// set a number of peaks for the major peaks module to find
#define NUM_PEAKS 2

VibrosonicsAPI vapi = VibrosonicsAPI();

float windowData[WINDOW_SIZE_BY_2] = { 0 };
float filteredData[WINDOW_SIZE_BY_2] = { 0 };
float longSmoothedData[WINDOW_SIZE_BY_2] = { 0 };
float shortSmoothedData[WINDOW_SIZE_BY_2] = { 0 };

Spectrogram rawSpectrogram(1);

Spectrogram processedSpectrogram = Spectrogram(2);
ModuleGroup melodic = ModuleGroup(&processedSpectrogram);
MajorPeaks majorPeaks = MajorPeaks(NUM_PEAKS);

Spectrogram percussiveSpectrogram = Spectrogram(2);
ModuleGroup percussive = ModuleGroup(&percussiveSpectrogram);
PercussionDetection percussionDetection = PercussionDetection();

FreqEnv dynamicFreqEnv = {};
AmpEnv dynamicAmpEnv = {};

void setup() {
  Serial.begin(115200);

  // call the API setup function
  vapi.init();

  melodic.addModule(&majorPeaks, 220, 1400);
  percussive.addModule(&percussionDetection, 1800, 4000);

  dynamicFreqEnv = vapi.createFreqEnv(110, 110, 110, 20);
  dynamicAmpEnv = vapi.createAmpEnv(0.5, 3, 0.5, 2, 0.4, 1, 0.0, 3, 1.0);
}

void loop() {
  // skip if new audio window has not been recorded
  if (!AudioLab.ready()) {
    return;
  }

  // process the raw audio signal into frequency domain data
  vapi.processAudioInput(windowData);

  // process the freqeuncy domain data
  vapi.noiseFloor(windowData, WINDOW_SIZE_BY_2, 220);
  
  // save the raw data for synthesis
  rawSpectrogram.pushWindow(windowData);

  // copy the windowData to filterdData
  memcpy(filteredData, windowData, WINDOW_SIZE_BY_2);

  // apply CFAR to filter the data
  vapi.noiseFloorCFAR(filteredData, WINDOW_SIZE_BY_2, 6, 1, 1.4);

  // smooth the filtered data over a long and short period of time
  AudioPrism::smooth_window_over_time(filteredData, longSmoothedData, 0.2);
  AudioPrism::smooth_window_over_time(filteredData, shortSmoothedData, 0.8);

  // push the short smoothed data for the melodic peak detection
  processedSpectrogram.pushWindow(shortSmoothedData);

  // subtract the long smoothed melodic data from the raw data to capture the
  // 'percussive data'
  for (int i = 0; i < WINDOW_SIZE_BY_2; i++) {
    windowData[i] = windowData[i] - longSmoothedData[i];
    if (windowData[i] < 0) {
      windowData[i] = 0;
    }
  }

  percussiveSpectrogram.pushWindow(windowData);

  // have analysis modules analyze the frequency domain data
  melodic.runAnalysis();
  percussive.runAnalysis();

  // create audio waves according the the output of the major peaks module
  synthesizePeaks(&majorPeaks, percussionDetection.getOutput());

  if (percussionDetection.getOutput()) {
    Serial.printf("Percussion detected\n");
    vapi.createDynamicGrain(1, SINE, dynamicFreqEnv, dynamicAmpEnv);
  } else {
    Serial.printf("\n");
  }

  // update the percussion grain
  vapi.updateGrains();

  // synthesize the waves created
  AudioLab.synthesize();

  //AudioLab.printWaves();
}

int interpolateAroundPeak(float* data, int indexOfPeak, int sampleRate, int windowSize) {
  float _freqRes = sampleRate * 1.0 / windowSize;
  float prePeak = indexOfPeak == 0 ? 0.0 : data[indexOfPeak - 1];
  float atPeak = data[indexOfPeak];
  float postPeak = indexOfPeak == WINDOW_SIZE_BY_2 ? 0.0 : data[indexOfPeak + 1];
  // summing around the index of maximum amplitude to normalize magnitudeOfChange
  float peakSum = prePeak + atPeak + postPeak;
  // interpolating the direction and magnitude of change, and normalizing from -1.0 to 1.0
  float magnitudeOfChange = ((atPeak + postPeak) - (atPeak + prePeak)) / (peakSum > 0.0 ? peakSum : 1.0);

  // return interpolated frequency
  return int(round((float(indexOfPeak) + magnitudeOfChange) * _freqRes));
}

// synthesizing should generally take from raw spectrum
//
// *this should not be copied as a standard function, is specialized for prax
// demo
void synthesizePeaks(MajorPeaks* peaks, int percussion) {
  float** peaksData = peaks->getOutput();
  // interpolate around peaks
  vapi.mapFrequenciesLinear(peaksData[MP_FREQ], NUM_PEAKS);
  vapi.mapAmplitudes(peaksData[MP_AMP], NUM_PEAKS, 10000);
  for (int i = 0; i < NUM_PEAKS; i++) {
    int freq = interpolateAroundPeak(rawSpectrogram.getCurrentWindow(), round(int(peaksData[MP_FREQ][i] * FREQ_WIDTH)), SAMPLE_RATE, WINDOW_SIZE);
    if (i == 0) {
      vapi.assignWave(freq, peaksData[MP_AMP][i], 1);
    } else if (percussion) {
      // vapi.assignWave(freq, peaksData[MP_AMP][i] / 2, 0);
    } else {
      // vapi.assignWave(freq, peaksData[MP_AMP][i], 0);
    }
  }
}
