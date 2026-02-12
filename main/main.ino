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

#include <VibrosonicsAPI.h>

// Vibrosonics audio analysis globals
VibrosonicsAPI vapi = VibrosonicsAPI();

#endif

// FreeRTOS stuff for the web server running on core 0
#define TASK_DELAY_MS 100u
#define WEB_SERVER_STACK_SIZE 8182u
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
  #endif
}

/**
 * @brief Audio analysis and synthesize running on core 1
 * 
 */
void loop()
{
#ifdef VAPI_EN
  // Check to make sure that the AudioLab input buffer has been filled
  if (!vapi.isAudioLabReady())
    return;

    // TODO: add VAPI processing here

#endif // VAPI_EN
}
