/***************************************************************
 * FILE: main.ino
 * 
 * DATE: 11/4/2025
 * 
 * DESCRIPTION: Entry point for starting the Vibrosonics audio
 * analysis and web app.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#include "webInterface.h"
#include "networking.h"
#include "fileSys.h"
#include "config.h"
#include "hapticSettings.h"
#include <memory>

#ifdef VAPI_EN

#include <VibrosonicsAPI.h>

// Vibrosonics audio analysis globals
VibrosonicsAPI vapi = VibrosonicsAPI();

float windowData[WINDOW_SIZE_BY_2] = { 0 };
float filteredData[WINDOW_SIZE_BY_2] = { 0 };
float smoothedData[WINDOW_SIZE_BY_2] = { 0 };
float melodicData[WINDOW_SIZE_BY_2] = { 0 };

Spectrogram melodicSpectrogram = Spectrogram(2, WINDOW_SIZE_OVERLAP);
ModuleGroup melodic = ModuleGroup(&melodicSpectrogram);

AnalysisModule* analysisModules[NUM_OUT_CH] = { nullptr };

#endif

// FreeRTOS stuff for the web server running on core 0
#define TASK_DELAY_MS 75u
#define WEB_SERVER_STACK_SIZE 8192u
#define WEB_SERVER_PRIORITY 3u
#define WEB_SERVER_CORE_ID 0u

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
  if (!HapticSettings::Instance().loadConfig())
  {
    DEBUG_PRINTLN("WARNING: Could not load previous analysis configuration from SD card");
    success = false;
  }
  // On setup failure, do nothing
  if (!success)
  {
    DEBUG_PRINTLN("FATAL: Setup failure. Looping...");

    while (true)
      delay(3000u);
  }
  #ifdef VAPI_EN
    DEBUG_PRINTLN("DEBUG: Initializing VAPI");
    
    // FIX: temporary
    auto loadedConfig = HapticSettings::Instance().getConfig_mut();
    assignOutputModules(loadedConfig);
    vapi.init();
  #endif
}

/**
 * @brief Audio analysis and synthesize running on core 1
 * 
 */
void loop()
{
#ifdef VAPI_EN
  if (!vapi.isAudioLabReady())
    return;
  
  auto activeConfig = HapticSettings::Instance().getConfig_r();

  // Get input data and clean it
  vapi.processAudioInput(windowData);
  vapi.noiseFloor(windowData, activeConfig->noiseFloor);
  memcpy(filteredData, windowData, WINDOW_SIZE_BY_2 * sizeof(float));
  vapi.noiseFloorCFAR(filteredData, activeConfig->cfarRefCount, activeConfig->cfarGuardCount, activeConfig->cfarBias);
  AudioPrism::smooth_window_over_time(filteredData, smoothedData, activeConfig->smoothingFactor, WINDOW_SIZE_OVERLAP);

  for (int i = 0; i < WINDOW_SIZE_BY_2; i++)
  {
    melodicData[i] = min(windowData[i], smoothedData[i]);
    
    if (melodicData[i] < activeConfig->noiseFloor)
      melodicData[i] = 0.;
  }
  melodicSpectrogram.pushWindow(melodicData);
  melodic.runAnalysis();

  for (int i = 0; i < NUM_OUT_CH; i++)
  {
    if (activeConfig->modules[i]->moduleType == MAJORPEAKS)
    {
      ModuleInterface<float**>* mpModuleInterface = static_cast<ModuleInterface<float**>*>(analysisModules[i]);
      float **analysisData = mpModuleInterface->getOutput();
      synthesizePeak(i, analysisData[MP_FREQ][0], analysisData[MP_AMP][0], activeConfig->modules[i]->freqLow, activeConfig->modules[i]->freqHigh, activeConfig->modules[i]->frequencyMapping);
      AudioLab.mapAmplitudes(i, activeConfig->modules[i]->minAmpNorm);
    }
  }
  AudioLab.synthesize();
#endif // VAPI_EN
}

#ifdef VAPI_EN

void assignOutputModules(std::shared_ptr<AnalysisConfig> target) {
  for (int i = 0; i < NUM_OUT_CH; i++){
    if (target->modules[i]->moduleType == MAJORPEAKS){
      MajorPeaksConfig* majorPeaks = static_cast<MajorPeaksConfig*>(target->modules[i]);
      analysisModules[i] = new MajorPeaks(majorPeaks->maxPeaks);
      analysisModules[i]->setWindowSize(WINDOW_SIZE_OVERLAP);
      melodic.addModule(analysisModules[i], target->modules[i]->freqLow, target->modules[i]->freqHigh);
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
  vapi.assignWave(haptic_freq, amp, channel);
}

#endif // VAPI_EN
