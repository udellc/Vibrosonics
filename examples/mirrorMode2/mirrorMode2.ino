#include "VibrosonicsAPI.h"

#define NUM_PEAKS 32

VibrosonicsAPI vapi = VibrosonicsAPI();

float windowData[WINDOW_SIZE_BY_2];
Spectrogram spectrogram = Spectrogram(1, WINDOW_SIZE_OVERLAP);
MajorPeaks majorPeaks = MajorPeaks(NUM_PEAKS);

void setup() {
  Serial.begin(115200);

  // call the API setup function
  vapi.init();

  majorPeaks.setWindowSize(WINDOW_SIZE_OVERLAP);

  majorPeaks.setSpectrogram(&spectrogram);
}

void loop() {
  // skip if new audio window has not been recorded
  if (!vapi.isAudioLabReady()) {
    return;
  }

  // process the raw audio signal into frequency domain data
  vapi.processAudioInput(windowData);

  for (int i = 0; i < WINDOW_SIZE_BY_2; i++) {
    if (windowData[i] < 300) {
      windowData[i] = 0;
    }
  }

  spectrogram.pushWindow(windowData);

  majorPeaks.doAnalysis();

  synthesizePeaks(&majorPeaks);

  AudioLab.synthesize();

  //AudioLab.printWaves();
}

int interpolateAroundPeak(float* data, int indexOfPeak) {
  float prePeak = indexOfPeak == 0 ? 0.0 : data[indexOfPeak - 1];
  float atPeak = data[indexOfPeak];
  float postPeak = indexOfPeak == WINDOW_SIZE_BY_2 ? 0.0 : data[indexOfPeak + 1];
  // summing around the index of maximum amplitude to normalize magnitudeOfChange
  float peakSum = prePeak + atPeak + postPeak;
  // interpolating the direction and magnitude of change, and normalizing from -1.0 to 1.0
  float magnitudeOfChange = ((atPeak + postPeak) - (atPeak + prePeak)) / (peakSum > 0.0 ? peakSum : 1.0);

  // return interpolated frequency
  return int(round((float(indexOfPeak) + magnitudeOfChange) * FREQ_RES));
}

// synthesizing should generally take from raw spectrum
void synthesizePeaks(MajorPeaks* peaks) {
  float** peaksData = peaks->getOutput();
  // interpolate around peaks
  vapi.mapAmplitudes(peaksData[MP_AMP], NUM_PEAKS);

  for (int i = 0; i < NUM_PEAKS; i++) {
    int freq = interpolateAroundPeak(spectrogram.getCurrentWindow(), round(int(peaksData[MP_FREQ][i] * FREQ_WIDTH)));
    vapi.assignWave(freq, peaksData[MP_AMP][i], 0);
    vapi.assignWave(freq, peaksData[MP_AMP][i], 1);
  }
}




    


  

  

  

  

  

  

  

  

  
 

