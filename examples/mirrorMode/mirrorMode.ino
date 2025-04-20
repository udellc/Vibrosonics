#include "VibrosonicsAPI.h"

// set a number of peaks for the major peaks module to find
#define NUM_PEAKS 8

VibrosonicsAPI vapi = VibrosonicsAPI();

float windowData[WINDOW_SIZE_BY_2];
Spectrogram rawSpectrogram = Spectrogram(2);
Spectrogram processedSpectrogram = Spectrogram(2);

ModuleGroup modules = ModuleGroup(&processedSpectrogram);
MajorPeaks majorPeaks = MajorPeaks(NUM_PEAKS);

void setup() {
  Serial.begin(115200);

  // call the API setup function
  vapi.init();

  // add the major peaks analysis module
  modules.addModule(&majorPeaks, 20, 3000);
}

void loop() {
  // skip if new audio window has not been recorded
  if (!AudioLab.ready()) {
    return;
  }

  // process the raw audio signal into frequency domain data
  vapi.processAudioInput(windowData);
  rawSpectrogram.pushWindow(windowData);

  // process the freqeuncy domain data
  vapi.noiseFloorCFAR(windowData, WINDOW_SIZE_BY_2, 4, 1, 1.8);
  processedSpectrogram.pushWindow(windowData);

  // have analysis modules analyze the frequency domain data
  modules.runAnalysis();

  // create audio waves according the the output of the major peaks module
  synthesizePeaks(&majorPeaks);

  // synthesize the waves created
  AudioLab.synthesize();

  //AudioLab.printWaves();
}

// synthesizing should generally take from raw spectrum
void synthesizePeaks(MajorPeaks* peaks) {
  float** peaksData = peaks->getOutput();
  // interpolate around peaks
  vapi.mapAmplitudes(peaksData[MP_AMP], NUM_PEAKS);
  vapi.assignWaves(peaksData[MP_FREQ], peaksData[MP_AMP], NUM_PEAKS, 0);
  vapi.assignWaves(peaksData[MP_FREQ], peaksData[MP_AMP], NUM_PEAKS, 1);
}
