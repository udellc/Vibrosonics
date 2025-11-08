#include "VibrosonicsAPI.h"

#define REF_CELLS 6
#define GUARD_CELLS 2
#define BIAS 1.4
#define NUM_PEAKS 4

VibrosonicsAPI vapi = VibrosonicsAPI();

float windowData[WINDOW_SIZE_BY_2];
Spectrogram rawSpectrogram = Spectrogram(2);
Spectrogram filteredSpectrogram = Spectrogram(2);

ModuleGroup rawModules = ModuleGroup(&rawSpectrogram);
Noisiness rawNoisiness = Noisiness();
MaxAmplitude rawMaxAmp = MaxAmplitude();
MeanAmplitude rawMeanAmp = MeanAmplitude();
MajorPeaks rawPeaks = MajorPeaks(NUM_PEAKS);

ModuleGroup filteredModules = ModuleGroup(&filteredSpectrogram);
Noisiness filteredNoisiness = Noisiness();
MaxAmplitude filteredMaxAmp = MaxAmplitude();
MeanAmplitude filteredMeanAmp = MeanAmplitude();
MajorPeaks filteredPeaks = MajorPeaks(NUM_PEAKS);

void setup() {
  Serial.begin(115200);
  vapi.init();

  rawModules.addModule(&rawNoisiness);
  rawModules.addModule(&rawMaxAmp);
  rawModules.addModule(&rawMeanAmp);
  rawModules.addModule(&rawPeaks);

  filteredModules.addModule(&filteredNoisiness);
  filteredModules.addModule(&filteredMaxAmp);
  filteredModules.addModule(&filteredMeanAmp);
  filteredModules.addModule(&filteredPeaks);
}


void loop() {
  if (!vapi.isAudioLabReady()) {
    return;
  }

  Serial.printf("New Window:\n");

  // process the audio signal with no noise filtering
  vapi.processAudioInput(windowData);
  rawSpectrogram.pushWindow(windowData);

  vapi.noiseFloorCFAR(windowData, REF_CELLS, GUARD_CELLS, BIAS);
  filteredSpectrogram.pushWindow(windowData);

  // have the analysis modules analyze the processed signal
  rawModules.runAnalysis();
  filteredModules.runAnalysis();

  // find the raw energy ratio and entropy (measure of uniformity) based on
  // analysis module output
  float rawEnergyRatio = rawMaxAmp.getOutput() / rawMeanAmp.getOutput();
  float rawEntropy = rawNoisiness.getOutput();

  // print the most significant peaks, energy ratio, and entropy of the raw
  // spectrum data
  Serial.printf("*Raw Data:\n");
  float** rawPeaksData = rawPeaks.getOutput();
  for (int i = 0; i < NUM_PEAKS; i++) {
    float freq = rawPeaksData[MP_FREQ][i];
    float amp = rawPeaksData[MP_AMP][i];

    Serial.printf("  (%f, %f)\n", freq, amp);
  }

  Serial.printf("  %f\n", rawEnergyRatio);
  Serial.printf("  %f\n", rawEntropy);

  // find the filtered energy ratio and entropy (measure of uniformity) based
  // on analysis module output
  float filteredEnergyRatio = filteredMaxAmp.getOutput() / filteredMeanAmp.getOutput();
  float filteredEntropy = filteredNoisiness.getOutput();

  // print the most significant peaks, energy ratio, and entropy of the
  // filtered spectrum data
  Serial.printf("*Filtered Data:\n");
  float** filteredPeaksData = filteredPeaks.getOutput();
  for (int i = 0; i < NUM_PEAKS; i++) {
    float freq = filteredPeaksData[MP_FREQ][i];
    float amp = filteredPeaksData[MP_AMP][i];

    Serial.printf("  (%f, %f)\n", freq, amp);
  }

  Serial.printf("  %f\n", filteredEnergyRatio);
  Serial.printf("  %f\n", filteredEntropy);

  int numFloored = 0;
  float* rawData = rawSpectrogram.getCurrentWindow();
  for (int i = 0; i < WINDOW_SIZE_BY_2; i++) {
    if (rawData[i] > 0 && windowData[i] == 0) {
      numFloored++;
    }
  }

  Serial.printf("# bins clrd = %d/%d\n", numFloored, WINDOW_SIZE_BY_2);
}
