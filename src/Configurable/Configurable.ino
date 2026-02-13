#include "VibrosonicsAPI.h"
#include "storage.h"

MajorPeaksConfig exampleMajorPeaksConfigs[4] = {
  { 400, 1000, OCTAVE, 10000, 1 },
  { 1000, 3600, OCTAVE, 10000, 1 },
  { 0, 4000, NONE, 10000, 32 },
  { 0, 4000, NONE, 10000, 32 },
};

AnalysisConfig exampleConfigs[3] = {
  {
    280,    // noise floor
    6,      // cfar number of reference cells
    1,      // cfar number of guard cells
    1.4,    // cfar bias
    0.2,    // smoothing factor 
    {
      &exampleMajorPeaksConfigs[0],
      &exampleMajorPeaksConfigs[1],
    }
  },
  {
    280,
    6,
    1,
    1.4,
    0.2,
    {
      &exampleMajorPeaksConfigs[1],
      &exampleMajorPeaksConfigs[0],
    }
  },
  {
    100,
    6,
    1,
    1.4,
    1,
    {
      &exampleMajorPeaksConfigs[2],
      &exampleMajorPeaksConfigs[3],
    }
  }
};

AnalysisConfig loadedConfig = exampleConfigs[0];

VibrosonicsAPI vapi = VibrosonicsAPI();

bool dirty = false;

float windowData[WINDOW_SIZE_BY_2] = { 0 };
float filteredData[WINDOW_SIZE_BY_2] = { 0 };
float smoothedData[WINDOW_SIZE_BY_2] = { 0 };
float melodicData[WINDOW_SIZE_BY_2] = { 0 };

Spectrogram melodicSpectrogram = Spectrogram(2, WINDOW_SIZE_OVERLAP);
ModuleGroup melodic = ModuleGroup(&melodicSpectrogram);

// NOTE: using float** for the type here is going to cause issues when adding percussion - find more elegant solution
ModuleInterface<float**>* analysisModules[NUM_OUT_CH] = { nullptr };

void setup() {
  Serial.begin(115200);

  // call the API setup function
  vapi.init();

  assignOutputModules();
}

void loop() {
  // NOTE: this is a placeholder for the web app configuration and will be deleted in the future
  if (Serial.available() > 0) {
    int exampleToLoad = Serial.parseInt();
    if (exampleToLoad >= 1 && exampleToLoad <= 3) {
      loadedConfig = exampleConfigs[exampleToLoad - 1];
      dirty = true;
    }
  }

  // skip if new audio window has not been recorded
  if (!vapi.isAudioLabReady()) {
    return;
  }

  // if output modules have changed since last window, need to reassign outputs
  if (dirty == true) {
    // delete old output modules
    clearOutputModules();

    // reassign modules to outputs
    assignOutputModules();

    dirty = false;
  }

  // process the raw audio signal into frequency domain data
  vapi.processAudioInput(windowData);

  // process the freqeuncy domain data

  // floor the noise from the wire using a set threshold
  vapi.noiseFloor(windowData, loadedConfig.noiseFloor);

  // copy the windowData to filterdData
  memcpy(filteredData, windowData, WINDOW_SIZE_BY_2 * sizeof(float));

  // apply CFAR to filter the data
  vapi.noiseFloorCFAR(filteredData, loadedConfig.cfarRefCount, loadedConfig.cfarGuardCount, loadedConfig.cfarBias);

  // smooth the filtered data over a long and short period of time
  AudioPrism::smooth_window_over_time(filteredData, smoothedData, loadedConfig.smoothingFactor, WINDOW_SIZE_OVERLAP);

  // calculate the percussive and melodic data
  for (int i = 0; i < WINDOW_SIZE_BY_2; i++) {
    // the smoothedData value is usually less than windowData's, but in the
    // case that windowData dropped quickly (becomes less than the
    // smoothedData) we want to adapt to that
    melodicData[i] = min(windowData[i], smoothedData[i]);
    if (melodicData[i] < loadedConfig.noiseFloor) {
      melodicData[i] = 0.;
    }
  }

  // push the short smoothed data for the melodic peak detection
  melodicSpectrogram.pushWindow(melodicData);

  // have analysis modules analyze the frequency domain data
  melodic.runAnalysis();

  for (int i = 0; i < NUM_OUT_CH; i++){
    if (loadedConfig.modules[i]->moduleType == MAJORPEAKS){
      float **analysisData = analysisModules[i]->getOutput();
      synthesizePeak(i, analysisData[MP_FREQ][0], analysisData[MP_AMP][0], loadedConfig.modules[i]->freqLow, loadedConfig.modules[i]->freqHigh, loadedConfig.modules[i]->frequencyMapping);
      AudioLab.mapAmplitudes(i, loadedConfig.modules[i]->minAmpNorm);
    }
  }

  // synthesize the waves created
  AudioLab.synthesize();
}

void clearOutputModules(){
  melodic.clearModules();
  // delete old modules
  for (int i = 0; i < NUM_OUT_CH; i++) {
    if (analysisModules[i] != nullptr){
      delete analysisModules[i];
      analysisModules[i] = nullptr;
    }
  }
}

void assignOutputModules(){
  for (int i = 0; i < NUM_OUT_CH; i++){
    if (loadedConfig.modules[i]->moduleType == MAJORPEAKS){
      MajorPeaksConfig* majorPeaks = static_cast<MajorPeaksConfig*>(loadedConfig.modules[i]);
      analysisModules[i] = new MajorPeaks(majorPeaks->maxPeaks);
      analysisModules[i]->setWindowSize(WINDOW_SIZE_OVERLAP);
      melodic.addModule(analysisModules[i], loadedConfig.modules[i]->freqLow, loadedConfig.modules[i]->freqHigh);
    }
  }
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

void synthesizePeak(int channel, float freq, float amp, float freqMin, float freqMax, FrequencyMapping mappingOption) {
  // interpolate the frequency around the peak to get a more accurate measure
  float interp_freq = interpolateAroundPeak(windowData, int(round(freq * FREQ_WIDTH)));

  float haptic_freq = interp_freq;
  if (mappingOption == OCTAVE)
  {
    haptic_freq = vapi.mapFrequencyByOctaves(interp_freq, freqMax);
  }
  else if (mappingOption == MIDI)
  {
    haptic_freq = vapi.mapFrequencyMIDI(interp_freq, freqMin, freqMax);
  }

  // create the wave
  vapi.assignWave(haptic_freq, amp, channel);
}
