#include "VibrosonicsAPI.h"

#define NOISE_FLOOR 280

#define MID_FREQ_LO 400
#define MID_FREQ_HI 1000
#define HIGH_FREQ_LO 1000
#define HIGH_FREQ_HI 3600  // specific frequency max for octave transposing, so we divide one less time compared to 4000.

VibrosonicsAPI vapi = VibrosonicsAPI();

float windowData[WINDOW_SIZE_BY_2] = { 0 };
float filteredData[WINDOW_SIZE_BY_2] = { 0 };
float smoothedData[WINDOW_SIZE_BY_2] = { 0 };
float melodicData[WINDOW_SIZE_BY_2] = { 0 };

Spectrogram melodicSpectrogram = Spectrogram(2);
ModuleGroup melodic = ModuleGroup(&melodicSpectrogram);
MajorPeaks midPeak = MajorPeaks(1);
MajorPeaks highPeak = MajorPeaks(1);

void setup() {
  Serial.begin(115200);

  // call the API setup function
  vapi.init();

  // set peak debug mode on to see the peaks picked up in each range
  // midPeak.setDebugMode(0x1);
  // highPeak.setDebugMode(0x1);

  // add melody peak modules to the melodic group
  melodic.addModule(&midPeak, MID_FREQ_LO, MID_FREQ_HI);
  melodic.addModule(&highPeak, HIGH_FREQ_LO, HIGH_FREQ_HI);
}

void loop() {
  // skip if new audio window has not been recorded
  if (!AudioLab.ready()) {
    return;
  }

  // process the raw audio signal into frequency domain data
  vapi.processAudioInput(windowData);

  // process the freqeuncy domain data

  // floor the noise from the wire using a set threshold
  vapi.noiseFloor(windowData, NOISE_FLOOR);

  // copy the windowData to filterdData
  memcpy(filteredData, windowData, WINDOW_SIZE_BY_2 * sizeof(float));

  // apply CFAR to filter the data
  vapi.noiseFloorCFAR(filteredData, 6, 1, 1.4);

  // smooth the filtered data over a long and short period of time
  AudioPrism::smooth_window_over_time(filteredData, smoothedData, 0.3);

  // calculate the percussive and melodic data
  for (int i = 0; i < WINDOW_SIZE_BY_2; i++) {
    // the smoothedData value is usually less than windowData's, but in the
    // case that windowData dropped quickly (becomes less than the
    // smoothedData) we want to adapt to that
    melodicData[i] = min(windowData[i], smoothedData[i]);
    if (melodicData[i] < NOISE_FLOOR) {
      melodicData[i] = 0.;
    }
  }

  // push the short smoothed data for the melodic peak detection
  melodicSpectrogram.pushWindow(melodicData);

  // have analysis modules analyze the frequency domain data
  melodic.runAnalysis();

  float **midPeakData = midPeak.getOutput();
  float **highPeakData = highPeak.getOutput();

  // synthesize the mid peak to the left speaker, ignoring percussion
  synthesizePeak(0, midPeakData[MP_FREQ][0], midPeakData[MP_AMP][0], MID_FREQ_HI);

  // synthesize the high peak to the right speaker, ducking with percussive
  // hits
  synthesizePeak(1, highPeakData[MP_FREQ][0], highPeakData[MP_AMP][0], HIGH_FREQ_HI);

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

void synthesizePeak(int channel, float freq, float amp, float freqMax) {
  // interpolate the frequency around the peak to get a more accurate measure
  float interp_freq = interpolateAroundPeak(windowData, int(round(freq * FREQ_WIDTH)));

  // map the frequency to the haptic range by dividing it by 2 (transposing by
  // octaves) until it is below 230Hz. This is why 3600Hz is a better max
  // frequency than 3800Hz+ since we can divide one less time and the output is
  // closer to the full haptic range.
  float haptic_freq = vapi.mapFrequencyByOctaves(interp_freq, 0, freqMax);

  // create the wave
 
