/***************************************************************
 * FILE: hapticSettings.cpp
 * 
 * DATE: 02/16/2026
 * 
 * DESCRIPTION: HapticSettings singleton class implementation
 * file.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#include "hapticSettings.h"
#include "config.h"
#include "fileSys.h"
#include "storage.h"
#include "utils.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <memory>

#define MAIN_ANALYSIS_PATH "/data/mainConfig.json"

// Queue stuff
#define QUEUE_LENGTH 12
#define ITEM_SIZE sizeof(QueueMessage)

static QueueHandle_t updateQueue;
static StaticQueue_t staticQueue;
uint8_t ucQueueStorageArea[ QUEUE_LENGTH * ITEM_SIZE ];

/**
 * @brief Plain constructor
 * 
 */
HapticSettings::HapticSettings() :
  curConfig{ nullptr }
{}

/**
 * @brief Loads an analysis config and inits the update queue
 * 
 * @return Boolean indicating if the settings are initialized properly 
 */
bool HapticSettings::init()
{
  loadConfig();

  updateQueue = xQueueCreateStatic(
    QUEUE_LENGTH,
    ITEM_SIZE,
    ucQueueStorageArea,
    &staticQueue
  );

  return (updateQueue != NULL);
}

/**
 * @brief Adds a message to the update queue
 * 
 * @param toSend - Pointer to the message to add
 * 
 * @return Bool indicating if the message could be added
 */
bool HapticSettings::addMessage(QueueMessage* toSend)
{
  return (xQueueSend(
    updateQueue,
    toSend,
    0
  ) == pdPASS);
}

/**
 * @brief Pops a message from the update queue
 * 
 * @param toGet - POinter to the message to be populated
 *
 * @return Bool indicating if a message could be popped 
 */
bool HapticSettings::getMessage(QueueMessage* toGet)
{
  return (xQueueReceive(
    updateQueue,
    toGet,
    0
  ) == pdPASS);
}

/**
 * @brief Checks if the queue has messages
 * 
 * @return Bool indicating if the queue has messages
 */
bool HapticSettings::needsUpdate()
{
  return (uxQueueMessagesWaiting(updateQueue) != 0);
}

/**
 * @brief Populates the analysis configurations using the data found in the
 *        SD card, falling back onto an internal preset of the data file is not
 *        found.
 *
 * @return Bool indicating if the loaded configuration was found from SD card.
 *         True if found on SD card.
 *         False if using preset.
 */
void HapticSettings::loadConfig()
{
  JsonDocument doc;
  bool usingSavedConfig {false};
  const bool HasConfig = FileSys::exists(MAIN_ANALYSIS_PATH);

  if (HasConfig)
  {
    File configFile = FileSys::getFile(MAIN_ANALYSIS_PATH);
    const auto Error = deserializeJson(doc, configFile);

    if (!Error)
    {
      this->curConfig = std::make_shared<AnalysisConfig>();
      JsonObject globalSettings = doc["global"].as<JsonObject>();
      JsonArray modulesList = doc["modules"].as<JsonArray>();

      Utils::populateGlobalSettings(globalSettings, this->curConfig.get());
      Utils::populateModulesList(modulesList, this->curConfig.get());

      usingSavedConfig = true;
    }
    else
      DEBUG_PRINTLN("WARNING: Saved analysis config could not be serialized");
  }
  if (!usingSavedConfig)
  {
    DEBUG_PRINTLN("WARNING: Using internal preset for analysis config");

    this->curConfig = std::make_shared<AnalysisConfig>();

    this->curConfig->noiseFloor = 280;
    this->curConfig->cfarRefCount = 6;
    this->curConfig->cfarGuardCount = 1;
    this->curConfig->cfarBias = 1.4;
    this->curConfig->smoothingFactor = 0.2;
    
    this->curConfig->modules[0] =
      std::make_unique<MajorPeaksConfig>(0, 1000, 3600, 10000.0, false, OCTAVE, 1);
    this->curConfig->modules[1] =
      std::make_unique<PercussionConfig>(1, 1800, 4000, 10000000.0, false, 0.5, 100000000.0, 0.78, TRIANGLE);
  }
}

/**
 * @brief Processes each message in the update queue
 * 
 * NOTE: This should be thread-safe since adding messages requires the instance, so it'll wait until
 *       this is done processing before adding. 
 * 
 * @return Bool indicating if the modules need to be rebuilt
 */
bool HapticSettings::processQueue(bool& sdSaveRequested)
{
  QueueMessage msg;
  bool needsRebuild = false;


  while (HapticSettings::Instance().getMessage(&msg))
  {
    switch (msg.id)
    {
      case QueueMsgId::EditGlobal:
        Utils::applyGlobalEdit(curConfig.get(), msg);
        break;

      case QueueMsgId::EditModule:
        needsRebuild |= Utils::applyModuleEdit(curConfig.get(), msg);
        break;

      case QueueMsgId::UpdateAll:
        needsRebuild = true;

      case QueueMsgId::CreateModule:
      {
        const int outputNumber = msg.module.outputNumber;
        
        if (outputNumber >= 0 && outputNumber < NUM_OUT_CH)
        {
          const auto Type = static_cast<ModuleType>(msg.module.value.i);
          auto newModule = Utils::createModule(Type);

          if (!newModule) break;

          newModule->outputNumber = outputNumber;
          curConfig->modules[outputNumber] = std::move(newModule);
          needsRebuild = true;
        }
        break;
      }
      case QueueMsgId::DeleteModule:
      {
        const int outputNumber = msg.module.outputNumber;
        
        if (outputNumber >= 0 && outputNumber < NUM_OUT_CH) {
          curConfig->modules[outputNumber] = nullptr;
          needsRebuild = true;
        }
        break;
      }
      case QueMsgId::SaveToSD:
      {
        sdSaveRequested = true;
        needsRebuild = true;
        break;
      }
      default:
        break;
    }
  }
  return needsRebuild;
}