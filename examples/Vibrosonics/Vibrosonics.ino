#include "VibrosonicsAPI.h"

#define NOISE_FLOOR 280

#define MID_FREQ_LO 400
#define MID_FREQ_HI 1000
#define HIGH_FREQ_LO 1000
#define HIGH_FREQ_HI 3800
#define PERC_FREQ_LO 1800
#define PERC_FREQ_HI 4000

VibrosonicsAPI vapi = VibrosonicsAPI();

float windowData[WINDOW_SIZE_BY_2] = { 0 };
float filteredData[WINDOW_SIZE_BY_2] = { 0 };
float moreSmoothedData[WINDOW_SIZE_BY_2] = { 0 };
float lessSmoothedData[WINDOW_SIZE_BY_2] = { 0 };
float percussiveData[WINDOW_SIZE_BY_2] = { 0 };
float melodicData[WINDOW_SIZE_BY_2] = { 0 };

Spectrogram rawSpectrogram(1, WINDOW_SIZE_OVERLAP);

Spectrogram melodicSpectrogram = Spectrogram(2, WINDOW_SIZE_OVERLAP);
ModuleGroup melodic = ModuleGroup(&melodicSpectrogram);
MajorPeaks midPeak = MajorPeaks(1);
MajorPeaks highPeak = MajorPeaks(1);

Spectrogram percussiveSpectrogram = Spectrogram(2, WINDOW_SIZE_OVERLAP);
ModuleGroup percussive = ModuleGroup(&percussiveSpectrogram);
PercussionDetection percussionDetection = PercussionDetection(0.5, 1800000, 0.75);
int windowsSinceHit = 0;

FreqEnv freqEnv = {};
AmpEnv ampEnv = {};
DurEnv durEnv = {};

void setup() {
  Serial.begin(115200);

  // call the API setup function
  vapi.init();

  // set peak debug mode on to see the peaks picked up in each range
  // midPeak.setDebugMode(0x1);
  // highPeak.setDebugMode(0x1);
  midPeak.setWindowSize(WINDOW_SIZE_OVERLAP);
  highPeak.setWindowSize(WINDOW_SIZE_OVERLAP);
  percussionDetection.setWindowSize(WINDOW_SIZE_OVERLAP);

  // add melody peak modules to the melodic group
  melodic.addModule(&midPeak, MID_FREQ_LO, MID_FREQ_HI);
  melodic.addModule(&highPeak, HIGH_FREQ_LO, HIGH_FREQ_HI);

  // set percussion debug mode on to configure the module's parameters
  // percussionDetection.setDebugMode(0x01);

  // add percussion detection module to the percussive group
  percussive.addModule(&percussionDetection, PERC_FREQ_LO, PERC_FREQ_HI);

  durEnv = vapi.createDurEnv(1, 0, 1, 3, 1.0);
}

void loop() {
  // skip if new audio window has not been recorded
  if (!vapi.isAudioLabReady()) {
    return;
  }

  // process the raw audio signal into frequency domain data
  vapi.processAudioInput(windowData);

  // process the freqeuncy domain data

  vapi.noiseFloor(windowData, NOISE_FLOOR);

  // save the raw data for synthesis
  rawSpectrogram.pushWindow(windowData);

  // copy the windowData to filterdData
  memcpy(filteredData, windowData, WINDOW_SIZE_BY_2 * sizeof(float));

  // apply CFAR to filter the data
  vapi.noiseFloorCFAR(filteredData, 6, 1, 1.4);

  // smooth the filtered data over a long and short period of time
  AudioPrism::smooth_window_over_time(filteredData, moreSmoothedData, 0.2, WINDOW_SIZE_OVERLAP);
  AudioPrism::smooth_window_over_time(filteredData, lessSmoothedData, 0.3, WINDOW_SIZE_OVERLAP);

  // calculate the percussive and melodic data
  for (int i = 0; i < WINDOW_SIZE_BY_2; i++) {
    // subtract the long smoothed melodic data from the raw data to capture the
    // 'percussive data'
    percussiveData[i] = max((float)0., windowData[i] - moreSmoothedData[i]);

    // the smoothedData value is usually less than windowData's, but in the
    // case that windowData dropped quickly (becomes less than the
    // smoothedData) we want to adapt to that
    melodicData[i] = min(windowData[i], lessSmoothedData[i]);
    if (melodicData[i] < NOISE_FLOOR) {
      melodicData[i] = 0.;
    }
  }

  // push the short smoothed data for the melodic peak detection
  melodicSpectrogram.pushWindow(melodicData);

  // experimental: floor more noise from the percussive data to better capture
  // hits after open hi-hats that leave energy leftover.
  // vapi.noiseFloor(percussiveData, 500);

  percussiveSpectrogram.pushWindow(percussiveData);

  // have analysis modules analyze the frequency domain data
  melodic.runAnalysis();
  percussive.runAnalysis();

  int p = percussionDetection.getOutput();

  float **midPeakData = midPeak.getOutput();
  float **highPeakData = highPeak.getOutput();

  // if percussion is detected, trigger a grain to synthesize the hit and reset
  // windowsSinceHit to 0.
  if (p) {
    float percussionAmp = AudioPrism::mean(windowData, PERC_FREQ_LO, PERC_FREQ_HI, WINDOW_SIZE_OVERLAP);
    // vapi.mapAmplitudes(&percussionAmp, 1);
    percussionAmp = percussionAmp * 5 + highPeakData[MP_AMP][0] * 1.1;
    freqEnv = vapi.createFreqEnv(160, 160, 160, 20);
    ampEnv = vapi.createAmpEnv(percussionAmp, percussionAmp, 0.3 * percussionAmp, 0.);
    vapi.createDynamicGrain(1, TRIANGLE, freqEnv, ampEnv, durEnv);

    percussionAmp *= 0.3;
    freqEnv = vapi.createFreqEnv(200, 200, 200, 20);
    ampEnv = vapi.createAmpEnv(percussionAmp, 0.9 * percussionAmp, 0.8 * percussionAmp, 0.);
    vapi.createDynamicGrain(1, TRIANGLE, freqEnv, ampEnv, durEnv);
    windowsSinceHit = 0;

    // Serial.printf("Percussion detected: %f amp\n", percussionAmp);
  }
  // otherwise, increment windowsSinceHit
  else {
    windowsSinceHit++;

    // Serial.printf("\n");
  }

  // float midEntropy = AudioPrism::entropy(melodicData, MID_FREQ_LO, MID_FREQ_HI);
  // float highEntropy = AudioPrism::entropy(melodicData, HIGH_FREQ_LO, HIGH_FREQ_HI);
  // Serial.printf("midEntropy: %f\n", midEntropy);
  // Serial.printf("highEntropy: %f\n", highEntropy);

  // synthesize the mid peak to the left speaker, ignoring percussion
  synthesizePeak(0, midPeakData[MP_FREQ][0], midPeakData[MP_AMP][0], 0);

  // synthesize the high peak to the right speaker, ducking with percussive
  // hits
  synthesizePeak(1, highPeakData[MP_FREQ][0], highPeakData[MP_AMP][0], 1);

  // update the percussion grain
  vapi.updateGrains();

  // map the amplitudes of waves in both channels
  AudioLab.mapAmplitudes(0, 10000);
  AudioLab.mapAmplitudes(1, 10000);

  // synthesize the waves created
  AudioLab.synthesize();

  // AudioLab.printWaves();
}

int interpolateAroundPeak(float *data, int indexOfPeak) {
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

void synthesizePeak(int channel, float freq, float amp, int hasPercussion) {
  // interpolate the frequency around the peak to get a more accurate measure
  float interp_freq = interpolateAroundPeak(windowData, int(round(freq * FREQ_WIDTH)));

  // map the frequency to the haptic range using MIDI note quantization
  float haptic_freq = vapi.mapFrequencyMIDI(interp_freq, MID_FREQ_LO, HIGH_FREQ_HI);

  // duck the amplitude to highlight percussive hits based on how long it has
  // been since the percussive hit. this should probably use a smooth
  // interpolation function instead.
  float adjusted_amp = amp;
  if (hasPercussion && windowsSinceHit < 1) {
    adjusted_amp /= 4;
  } else if (hasPercussion && windowsSinceHit < 2) {
    adjusted_amp /= 3;
  } else if (hasPercussion && windowsSinceHit < 3) {
    adjusted_amp /= 2;
  } else if (hasPercussion && windowsSinceHit < 4) {
    adjusted_amp /= 1.5;
  } else if (hasPercussion && windowsSinceHit < 5) {
    adjusted_amp /= 1.2;
  }

  // create the wave
  vapi.assignWave(haptic_freq, adjusted_amp, channel);
}
