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

#include "webServer.h"
#include "networking.h"
#include "fileSys.h"
#include "config.h"
#include "VibrosonicsAPI.h"

#define VIBROSONICS_STACK_SIZE 8192u
#define NUM_PEAKS 4
#define LOW_FREQ 20
#define HIGH_FREQ 3000

// ESP32 Config globals
StaticTask_t vibrosonicsTaskBuffer;
StackType_t vibrosonicsStack[VIBROSONICS_STACK_SIZE];

// Vibrosonics audio analysis globals
static VibrosonicsAPI vapi = VibrosonicsAPI();
float windowData[WINDOW_SIZE_BY_2];
Spectrogram processedSpectrogram = Spectrogram(2, WINDOW_SIZE_OVERLAP);
ModuleGroup modules = ModuleGroup(&processedSpectrogram);
MajorPeaks majorPeaks = MajorPeaks(NUM_PEAKS);

void runVibrosonicsTask(void *pvParams);

/**
 * @brief 
 * 
 */
void setup()
{
  bool success = true;

  Serial.begin(115200);
  success &= FileSys::init();
  success &= Networking::init();
  success &= WebServer::init();

  // On setup failure, do nothing 
  if (!success)
  {
    Serial.println("Setup failure. Looping...");
    
    while (true)
      delay(1000);
  }
  const auto Created = xTaskCreateStaticPinnedToCore
  (
    runVibrosonicsTask,
    "VibrosonicsTask",
    VIBROSONICS_STACK_SIZE,
    nullptr,
    5u,
    vibrosonicsStack,
    &vibrosonicsTaskBuffer,
    0
  );
  if (!Created)
  {
    Serial.println("Failed to create task, looping...");
    while(true)
      delay(2000);
  }
  vapi.init();
  majorPeaks.setWindowSize(WINDOW_SIZE_OVERLAP);
  modules.addModule(&majorPeaks, 20, 3000);
}

/**
 * @brief 
 * 
 */
void loop()
{
  WebServer::updateServer();
}

void runVibrosonicsTask(void *pvParams)
{
  while (true)
  {
    if (!vapi.isAudioLabReady())
    {
      vTaskDelay(1);
    }
    else
    {
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
      // Print out peak data
      // Serial.printf("Major Peaks:\n");
      // for (int i = 0; i < NUM_PEAKS; i++)
      // {
      //   Serial.printf("Peak: %i Frequency: %fHz Amplitude: %f\n", i, FREQ_RES * peaksData[MP_FREQ][i], peaksData[MP_AMP][i]);
      // }
      // Generate waves to be outputted on the hardware on channel 0
      vapi.assignWaves(peaksData[MP_FREQ], peaksData[MP_AMP], NUM_PEAKS, 0);
      // Synthesize all created waves through AudioLab
      AudioLab.synthesize();
    }
  }
}
