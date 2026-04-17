/***************************************************************
 * FILE: main.ino
 * 
 * DATE: 2/22/2026
 * 
 * DESCRIPTION: Entry point for starting the Vibrosonics audio
 * analysis and web app.
 * 
 * AUTHOR: Ivan Wong, Danielle Chang
 ***************************************************************/

#include "webInterface.h"
#include "networking.h"
#include "fileSys.h"
#include "config.h"
#include "hapticSettings.h"
#include <memory>
#include <VibrosonicsAPI.h>

// Vibrosonics audio analysis globals
VibrosonicsAPI vapi = VibrosonicsAPI();
std::shared_ptr<AnalysisConfig> activeConfig {nullptr};

float windowData[WINDOW_SIZE_BY_2] = { 0 };
float filteredData[WINDOW_SIZE_BY_2] = { 0 };
float smoothedData[WINDOW_SIZE_BY_2] = { 0 };
float percussionSmoothedData[WINDOW_SIZE_BY_2] = { 0 };
float melodicData[WINDOW_SIZE_BY_2] = { 0 };
float percussiveData[WINDOW_SIZE_BY_2] = { 0 };

Spectrogram melodicSpectrogram = Spectrogram(2, WINDOW_SIZE_OVERLAP);
ModuleGroup melodic = ModuleGroup(&melodicSpectrogram);
Spectrogram percussiveSpectrogram = Spectrogram(2, WINDOW_SIZE_OVERLAP);
ModuleGroup percussive = ModuleGroup(&percussiveSpectrogram);

FreqEnv freqEnv = {};
AmpEnv ampEnv = {};
DurEnv durEnv = {};

int windowsSinceHit = 0;

// list of our analysis modules. the maximum number of possible modules is currently
// NUM_OUT_CH * 2 as each output can have one non-percussion module and one percussion module
AnalysisModule* analysisModules[NUM_OUT_CH * 2] = { nullptr };
bool outputHasPercussion[NUM_OUT_CH] = { false };

// FreeRTOS stuff for the web server running on core 0
#define TASK_DELAY_MS 100u
#define WEB_SERVER_STACK_SIZE 8192u
#define WEB_SERVER_PRIORITY 3u
#define WEB_SERVER_CORE_ID 0u

// Helper functions
void processData(std::shared_ptr<const AnalysisConfig> target);
void performModuleAnalysis(AnalysisModule* module, const ModuleConfig* moduleConfig);
inline void doPercussiveAnalysis(AnalysisModule* module, const ModuleConfig* moduleConfig);
inline void doMajorPeaksAnalysis(AnalysisModule* module, const ModuleConfig* moduleConfig);
void rebuildOutputModules(const AnalysisConfig* Config);
int interpolateAroundPeak(float *data, int indexOfPeak);
void synthesizePeak(int channel, float freq, float amp, float freqMin, float freqMax, FrequencyMapping mappingOption);

/**
 * @brief Function to be pinned to core 0. Handles the clients for the web server
 *        every TASK_DELAY_MS.
 * 
 * @param params - Parameters used
 * 
 * NOTE: params is UNUSED but needed to match the function signature for
 *       xTaskCreatePinnedToCore()
 */
void webRunner(void *params)
{
  while (true)
  {
    WebInterface::run();
    vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
  }
}

/**
 * @brief Initializes system components
 * 
 */
void setup()
{
  bool success = true;
  DEBUG_BEGIN(115200);
  success &= FileSys::init();
  success &= Networking::init();

  // NOTE: Only fails if index.html is not found on SD card. If we're in dev
  //       mode, we don't care if the web app is on it or not
#ifndef DEV_MODE_EN
  success &= WebInterface::init();
#else
  (void) WebInterface::init();
#endif

  success &= HapticSettings::Instance().init();

  const auto CreatedTask = xTaskCreatePinnedToCore(
    webRunner,
    "webServer",
    WEB_SERVER_STACK_SIZE,
    NULL,                 // Input params, NULL b/c we don't use any
    WEB_SERVER_PRIORITY,  // Higher the num, higher the priority
    NULL,                 // TaskHandle_T*, not using, so it's NULL
    WEB_SERVER_CORE_ID
  );
  if (CreatedTask != pdPASS)
  {
    success = false;
    DEBUG_PRINTLN("FATAL: Could not create web server task");
  }
  // On setup failure, do nothing
  if (!success)
  {
    DEBUG_PRINTLN("FATAL: Setup failure. Looping...");

    while (true)
      delay(3000u);
  }
  DEBUG_PRINTLN("DEBUG: Initializing VAPI");

  activeConfig = HapticSettings::Instance().getConfig_mut();
  rebuildOutputModules(activeConfig.get());
  durEnv = vapi.createDurEnv(1, 0, 1, 3, 1.0);

  vapi.init();
}

/**
 * @brief Audio analysis and synthesize running on core 1
 * 
 */
void loop()
{
  if (!vapi.isAudioLabReady())
    return;

  if (HapticSettings::Instance().needsUpdate())
  {
    DEBUG_PRINTLN("DEBUG: processing queue...");

    // NOTE: only returns true when it actually needs to be rebuilt, not every time it processes a request
    if (HapticSettings::Instance().processQueue())
    {
      // Get the most recent config incase some were deleted or added
      activeConfig = HapticSettings::Instance().getConfig_mut();
      rebuildOutputModules(activeConfig.get());
    }
  }
  processData(activeConfig);

  melodic.runAnalysis();
  percussive.runAnalysis();

  for (int i = 0; i < NUM_OUT_CH * 2; i++)
  {
    if (analysisModules[i] && activeConfig->modules[i]) {
      performModuleAnalysis(
        analysisModules[i],
        activeConfig->modules[i].get()
      );
    }
  }
  vapi.updateGrains();

  for (int ch = 0; ch < NUM_OUT_CH; ch++)
  {
      AudioLab.mapAmplitudes(ch, activeConfig->minAmpNorm);
  }
  AudioLab.synthesize();
}

void processData(std::shared_ptr<const AnalysisConfig> target){
  // Get input data and clean it
  vapi.processAudioInput(windowData);
  vapi.noiseFloor(windowData, target->noiseFloor);
  memcpy(filteredData, windowData, WINDOW_SIZE_BY_2 * sizeof(float));
  vapi.noiseFloorCFAR(filteredData, target->cfarRefCount, target->cfarGuardCount, target->cfarBias);
  AudioPrism::smooth_window_over_time(filteredData, smoothedData, target->smoothingFactor, WINDOW_SIZE_OVERLAP);
  // smooth data being used to create percussive data. we don't want the smoothing factor to be adjustable here
  AudioPrism::smooth_window_over_time(filteredData, percussionSmoothedData, 0.2, WINDOW_SIZE_OVERLAP);

  for (int i = 0; i < WINDOW_SIZE_BY_2; i++)
  {
    percussiveData[i] = max((float)0., windowData[i] - percussionSmoothedData[i]);
    
    melodicData[i] = min(windowData[i], smoothedData[i]);
    if (melodicData[i] < target->noiseFloor)
      melodicData[i] = 0.;
  }
  melodicSpectrogram.pushWindow(melodicData);
  percussiveSpectrogram.pushWindow(percussiveData);
}

inline void doPercussiveAnalysis(AnalysisModule* module, const ModuleConfig* moduleConfig)
{
  auto percModuleInterface = static_cast<ModuleInterface<bool>*>(module);
  auto percussionConfig = static_cast<const PercussionConfig*>(moduleConfig);
  // If percussion was detected, synthesize a hit
  if (percModuleInterface->getOutput()) {
    DEBUG_PRINTLN("Percussion hit detected");
    // Get the energy, entropy and positive flux for the percussive hit. These
    // values are used to synthesize the haptic feedback of the percussion.
    float energy = AudioPrism::energy(percussiveData, 
                                      moduleConfig->freqLow, 
                                      moduleConfig->freqHigh, 
                                      WINDOW_SIZE_OVERLAP);
    float entropy = AudioPrism::entropy(percussiveData, 
                                      moduleConfig->freqLow, 
                                      moduleConfig->freqHigh, 
                                      WINDOW_SIZE_OVERLAP);
    float flux = AudioPrism::positive_flux(percussiveData,
                                      percussiveSpectrogram.getPreviousWindow(),
                                      moduleConfig->freqLow, 
                                      moduleConfig->freqHigh, 
                                      WINDOW_SIZE_OVERLAP);

    // Normalize the flux [0.0, 1.0] by the total energy
    if (energy > 0.0f)
        flux /= energy;
    else
        flux = 0.0f;

    // Create the frequency and amplitude envelopes for the percussive hit,
    // using a set frequency of 160 and the energy of the detected hit as the
    // amplitude.
    freqEnv = vapi.createFreqEnv(160, 160, 160, 20);
    ampEnv = vapi.createAmpEnv(energy, energy, 0.3 * energy, 0.);

    vapi.createDynamicGrain(moduleConfig->outputNumber, 
                            percussionConfig->waveType, 
                            freqEnv, 
                            ampEnv, 
                            durEnv);

    // For particularily noisy hits, synthesize another hit with less energy
    // to create a rougher feeling.
    if (entropy > 0.9) {
      energy *= 0.3;
      freqEnv = vapi.createFreqEnv(200, 200, 200, 20);
      ampEnv = vapi.createAmpEnv(energy, energy, 0.3 * energy, 0.);
      vapi.createDynamicGrain(moduleConfig->outputNumber, 
                              percussionConfig->waveType, 
                              freqEnv, 
                              ampEnv, 
                              durEnv);
    }
    windowsSinceHit = 0;
  }
  else {
    windowsSinceHit++;
  }
}

inline void doMajorPeaksAnalysis(AnalysisModule* module, const ModuleConfig* moduleConfig)
{
  auto mpModuleInterface = static_cast<ModuleInterface<float**>*>(module);
  auto majorPeaksConfig = static_cast<const MajorPeaksConfig*>(moduleConfig);
  float **analysisData = mpModuleInterface->getOutput();

  for (auto i {0}; i < majorPeaksConfig->maxPeaks; i++)
  {
    synthesizePeak(moduleConfig->outputNumber, 
                    analysisData[MP_FREQ][i], 
                    analysisData[MP_AMP][i], 
                    moduleConfig->freqLow, 
                    moduleConfig->freqHigh, 
                    majorPeaksConfig->frequencyMapping);
  }
}

void performModuleAnalysis(AnalysisModule* module, const ModuleConfig* moduleConfig){
  switch(moduleConfig->moduleType){
    case PERCUSSION:
      doPercussiveAnalysis(module, moduleConfig);
      break;

    case MAJORPEAKS:
      doMajorPeaksAnalysis(module, moduleConfig);
      break;

    default:
      break;
  }
}

void rebuildOutputModules(const AnalysisConfig* Config)
{
  melodic.clearModules();
  percussive.clearModules();

  for (int ch = 0; ch < NUM_OUT_CH; ch++)
    outputHasPercussion[ch] = false;

  for (int i = 0; i < NUM_OUT_CH * 2; i++)
  {
    if (analysisModules[i])
    {
      delete analysisModules[i];
      analysisModules[i] = nullptr;
    }

    if (!Config->modules[i])
      continue;

    switch(Config->modules[i]->moduleType){
      case MAJORPEAKS:
      {
        auto* majorPeaks = static_cast<MajorPeaksConfig*>(Config->modules[i].get());

        analysisModules[i] = new MajorPeaks(majorPeaks->maxPeaks);
        analysisModules[i]->setWindowSize(WINDOW_SIZE_OVERLAP);
        melodic.addModule(analysisModules[i], Config->modules[i]->freqLow, Config->modules[i]->freqHigh);
        break;
      }
      case PERCUSSION:
      {
        auto* percussionConfig = static_cast<PercussionConfig*>(Config->modules[i].get());

        analysisModules[i] = new PercussionDetection(percussionConfig->fluxThresh, 
                                                    percussionConfig->energyThresh, 
                                                    percussionConfig->entropyThresh);
        analysisModules[i]->setWindowSize(WINDOW_SIZE_OVERLAP);
        percussive.addModule(analysisModules[i], Config->modules[i]->freqLow, Config->modules[i]->freqHigh);
        outputHasPercussion[Config->modules[i].get()->outputNumber] = true;
        break;
      }
      default:
        break;
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

inline float linear_interpolation(float a, float b, float t) {
  return a + t * (b - a);
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

  // duck the amplitude to highlight percussive hits based on how long it has
  // been since the percussive hit
  float adjusted_amp = amp;
  if (outputHasPercussion[channel]) {
    const float minDiv = 4.0f;
    const float maxDiv = 1.0f;
    const float maxWindows = 5.0f;

    // uses linear interpolation to determine how much to duck the amplitude
    float t = min((float)windowsSinceHit / maxWindows, 1.0f);
    float divisor = linear_interpolation(minDiv, maxDiv, t);

    adjusted_amp /= divisor;
  }

  vapi.assignWave(haptic_freq, adjusted_amp, channel);
}
