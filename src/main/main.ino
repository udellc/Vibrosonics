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

#ifdef VAPI_EN

#include "VibrosonicsAPI.h"
#define NUM_PEAKS 12

// Vibrosonics audio analysis globals
VibrosonicsAPI vapi = VibrosonicsAPI();
float windowData[WINDOW_SIZE_BY_2];
Spectrogram processedSpectrogram = Spectrogram(2, WINDOW_SIZE_OVERLAP);
ModuleGroup modules = ModuleGroup(&processedSpectrogram);
MajorPeaks majorPeaks = MajorPeaks(NUM_PEAKS);

#endif

// FreeRTOS stuff for the web server running on core 0
#define TASK_DELAY_MS 100u
#define WEB_SERVER_STACK_SIZE 8182u
#define WEB_SERVER_PRIORITY 3u
#define WEB_SERVER_CORE_ID 0u

// TODO: add header comment
void webRunner(void *params)
{
  while (true)
  {
    WebInterface::run();
    vTaskDelay(pdMS_TO_TICKS(TASK_DELAY_MS));
  }
}

/**
 * @brief 
 * 
 */
void setup()
{
  bool success = true;
  DEBUG_BEGIN(115200);
  success &= FileSys::init();
  success &= Networking::init();
  success &= WebInterface::init();

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
    DEBUG_PRINTLN("DEBUG: Could not create web server task");
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
    vapi.init();
    majorPeaks.setWindowSize(WINDOW_SIZE_OVERLAP);
    modules.addModule(&majorPeaks, 20, 3000);
  #endif
}

/**
 * @brief 
 * 
 */
void loop()
{
#ifdef VAPI_EN
  // Check to make sure that the AudioLab input buffer has been filled
  if (!vapi.isAudioLabReady())
  {
    return;
  }
  // Process the input data
  vapi.processAudioInput(windowData);

  // Using this noise flooring function helps with getting a clear
  // sounding output. This is more useful on the original prototype.
  // You may not need this if you are using the latest hardware.
  vapi.noiseFloorCFAR(windowData, 4, 1, 1.6);

  // Push the processed data to the processed spectrogram
  processedSpectrogram.pushWindow(windowData);

  // Analyze the data with the added AudioPrism modules
  modules.runAnalysis();

  // Get the analyzed data from MajorPeaks module
  float** peaksData = majorPeaks.getOutput();

  /**
   * Now that we have the data from MajorPeaks' analysis, we can
   * decide what to do with that data.
   * Some examples of what to do with the data are:
   * -- Output the wave peaks
   * -- Print out the info about the found peaks
   * Both of these examples are shown below
   */

  // Print out peak data
  // Serial.printf("Major Peaks:\n");
  // for (int i = 0; i < NUM_PEAKS; i++){
  //   Serial.printf("Peak: %i Frequency: %fHz Amplitude: %f\n", i, FREQ_RES * peaksData[MP_FREQ][i], peaksData[MP_AMP][i]);
  // }

  // Generate waves to be outputted on the hardware on channel 0
  vapi.assignWaves(peaksData[MP_FREQ], peaksData[MP_AMP], NUM_PEAKS, 0);

  // Synthesize all created waves through AudioLab
  AudioLab.synthesize();
#endif // VAPI_EN
}
