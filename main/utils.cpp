/***************************************************************
 * FILE: utils.cpp
 * 
 * DATE: 2/19/2026
 * 
 * DESCRIPTION: Implementation file for the Utils namespace.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#include "config.h"
#include "storage.h"
#include <map>
#include <functional>
#include <memory>
#include "utils.h"

using CreateModule = std::function<ModulePtr()>;
using ModuleFactory = std::map<ModuleType, CreateModule>;

/**
 * @brief Populates the global analysis configuration settings using
 *        the JSON structure found in /WebApp/src/data.
 * 
 * @param global - Reference to the JSON object to get the global data from.
 * @param config - Pointer to the AnalysisConfig to update.
 */
void Utils::populateGlobalSettings(JsonObject& global, AnalysisConfig* config)
{
  config->noiseFloor = global["noiseFloor"] | 280.0f;
  config->cfarRefCount = global["cfarRefCount"] | 6;
  config->cfarGuardCount = global["cfarGuardCount"] | 1;
  config->cfarBias = global["cfarBias"] | 1.2f;
  config->smoothingFactor = global["smoothingFactor"] | 0.2f;
}

/**
 * @brief Populates the analysis config with the JSON array holding the
 *        modules using the base structure found in /WebApp/src/data and
 *        derived structures found in /main/storage.h
 * 
 * @param modulesList - Reference to the JSON array holding the modules info
 * @param config - Pointer to the AnalysisConfig to update
 */
void Utils::populateModulesList(JsonArray& modulesList, AnalysisConfig* config)
{
  // Channel count
  int ch {0};

  for (auto module : modulesList)
  {
    if (ch >= (NUM_OUT_CH)) break;

    // newModule type is ModuleConfig at the moment
    const auto Type = static_cast<ModuleType>(module["moduleType"]);
    auto newModule = createModule(Type);

    if (!newModule)
    {
      DEBUG_PRINTLN("WARNING: Memory could not be allocated for new module in populateModulesList");
      continue;
    }
    // Update base data
    auto* modulePtr = newModule.get();
    modulePtr->outputNumber = module["outputNumber"]; 
    modulePtr->freqLow = module["freqLow"]; 
    modulePtr->freqHigh = module["freqHigh"];
    modulePtr->minAmpNorm = module["minAmpNorm"];
    modulePtr->isMuted = module["isMuted"];

    // Do the rest params under a function that takes in the type or use branching, for now just do this for MajorPeaks
    if (Type == MAJORPEAKS)
    {
      auto* majorPeaksConfig = static_cast<MajorPeaksConfig*>(newModule.get());

      majorPeaksConfig->frequencyMapping = static_cast<FrequencyMapping>(module["frequencyMapping"]);
      majorPeaksConfig->maxPeaks = module["maxPeaks"];
    }
    else if (Type == PERCUSSION)
    {
      auto* percussionConfig = static_cast<PercussionConfig*>(newModule.get());

      percussionConfig->fluxThresh = module["fluxThresh"];
      percussionConfig->energyThresh = module["energyThresh"];
      percussionConfig->entropyThresh = module["entropyThresh"];
      percussionConfig->waveType = static_cast<WaveType>(module["waveType"]);
    }
    // The old module memory will be deleted automatically since the ptr now has no references
    config->modules[ch] = std::move(newModule);
    ch++;
  }
}

/**
 * @brief Populates the referenced JSON object with global data from the
 *        AnalysisConfig using the JSON structure found in /WebApp/src/data.
 * 
 * @param global - Reference to the JSON object to be populated.
 * @param config - Pointer to the AnalysisConfig to pull data from.
 */
void Utils::packageGlobalSettings(JsonObject& global, AnalysisConfig* config)
{
  global["noiseFloor"] = config->noiseFloor;
  global["cfarRefCount"] = config->cfarRefCount;
  global["cfarGuardCount"] = config->cfarGuardCount;
  global["cfarBias"] = config->cfarBias;
  global["smoothingFactor"] = config->smoothingFactor;
}

/**
 * @brief Populates the references JSON array with module data from the
 *        AnalysisConfig using the JSON structure found in /WebApp/src/data.
 * 
 * @param modulesList - Reference to the modules array to populate.
 * @param config - Pointer to the AnalysisConfig to pull data from.
 */
void Utils::packageModulesList(JsonArray& modulesList, AnalysisConfig* config)
{
 for (auto i {0u}; i < NUM_OUT_CH; i++)
 {
  if (config->modules[i] == nullptr)
    continue;

  // Allocate memory in the array
  JsonObject module = modulesList.add<JsonObject>();

  module["outputNumber"] = config->modules[i]->outputNumber;
  module["moduleType"] = config->modules[i]->moduleType;
  module["freqLow"] = config->modules[i]->freqLow;
  module["freqHigh"] = config->modules[i]->freqHigh;
  module["minAmpNorm"] = config->modules[i]->minAmpNorm;
  module["isMuted"] = config->modules[i]->isMuted;

  // Do the rest params under a function that takes in the type or use branching, for now just do this for MajorPeaks
  if (config->modules[i]->moduleType == MAJORPEAKS)
  {
    auto* modulePtr = static_cast<MajorPeaksConfig*>(config->modules[i].get());
    module["frequencyMapping"] = modulePtr->frequencyMapping;
    module["maxPeaks"] = modulePtr->maxPeaks;
  }
  else if (config->modules[i]->moduleType == PERCUSSION)
  {
    auto* modulePtr = static_cast<PercussionConfig*>(config->modules[i].get());
    module["fluxThresh"] = modulePtr->fluxThresh;
    module["energyThresh"] = modulePtr->energyThresh;
    module["entropyThresh"] = modulePtr->entropyThresh;
    module["waveType"] = modulePtr->waveType;
  }
 } 
}

/**
 * @brief Creates a unique pointer of the passed in module type using
 *        the factory method w/ a map.
 * 
 * @param Type - Type of module config to create.
 *
 * @return ModulePtr - Unique pointer of the passed in module type.
 */
inline ModulePtr Utils::createModule(const ModuleType Type)
{
  // Used to create default module configs
  static const ModuleFactory Map =
  {
    { MAJORPEAKS, []() { return std::make_unique<MajorPeaksConfig>(0, 400, 1000, 10000.0, false, OCTAVE, 1); } },
    { PERCUSSION, []() { return std::make_unique<PercussionConfig>(0, 1800, 4000, 10000000.0, false, 0.5, 100000000.0, 0.78, TRIANGLE); } },
  };
  // If the module type is defined, we return the second element since the Map carries a pair:
  // Ex: ( ModuleType, unique_ptr for module )
  auto itr = Map.find(Type);
  return (itr == Map.end() ? nullptr : itr->second());
}

/**
 * @brief Creates a message from the JsonObject data
 * 
 * @param id - Message id to use
 * @param payload - Reference to the data source
 * @param msg - Message structure to be populated
 */
void Utils::createMessage(const QueueMsgId id, const JsonObject& payload, QueueMessage& msg)
{
  msg.id = id;
  msg.field = static_cast<ConfigField>(payload["field"].as<uint>());

  switch (id)
  {
    case QueueMsgId::EditGlobal:
    {
      switch (msg.field)
      {
        case ConfigField::CfarRefCount:
        case ConfigField::CfarGuardCount:
          msg.global.value.u16 = payload["value"].as<uint16_t>();
          break;

        case ConfigField::NoiseFloor:
        case ConfigField::CfarBias:
        case ConfigField::SmoothingFactor:
          msg.global.value.f = payload["value"].as<float>();
          break;

        default:
          break;
      }
      break;
    }
    case QueueMsgId::EditModule:
    {
      msg.module.outputNumber = payload["outputNumber"].as<int>();

      switch (msg.field)
      {
        case ConfigField::FreqLow:
        case ConfigField::FreqHigh:
          msg.module.value.u16 = payload["value"].as<uint16_t>();
          break;

        case ConfigField::OutputNumber:
        case ConfigField::MaxPeaks:
          msg.module.value.i = payload["value"].as<int>();
          break;

        case ConfigField::FrequencyMapping:
          msg.module.value.fm = payload["value"].as<FrequencyMapping>();
          break;

        case ConfigField::WaveType:
          msg.module.value.wt = payload["value"].as<WaveType>();
          break;

        case ConfigField::FluxThresh:
        case ConfigField::EnergyThresh:
        case ConfigField::EntropyThresh:
        case ConfigField::MinAmpNorm:
          msg.module.value.f = payload["value"].as<float>();
          break;

        case ConfigField::IsMuted:
          msg.module.value.b = payload["value"].as<bool>();
          break;

        default:
          break;
      }
    }
    case QueueMsgId::UpdateAll:   // Just needs the ID for a rebuild
    default:
      break;
  }
}

/**
 * @brief Updates the global config field using the message
 * 
 * @param config - Config pointer to be edited
 * @param msg - Message holding which field and what value to use
 */
void Utils::applyGlobalEdit(AnalysisConfig* config, const QueueMessage& msg)
{
  switch (msg.field)
  {
    case ConfigField::NoiseFloor: 
      config->noiseFloor = msg.global.value.f;
      break;

    case ConfigField::CfarRefCount:
      config->cfarRefCount = msg.global.value.u16;
      break;

    case ConfigField::CfarGuardCount:  
      config->cfarGuardCount  = msg.global.value.u16;
      break;

    case ConfigField::CfarBias:        
      config->cfarBias = msg.global.value.f;
      break;

    case ConfigField::SmoothingFactor: 
      config->smoothingFactor = msg.global.value.f;
      break;

    default:
      break;
  }
}

/**
 * @brief Updates the module config within the analysis config
 * 
 * @param config - Config pointer to edit
 * @param msg - Message holding which output, field, and what value to use
 *
 * @return Bool indicating if the modules need to be rebuilt in the main loop 
 */
bool Utils::applyModuleEdit(AnalysisConfig* config, const QueueMessage& msg)
{
  if (!config->modules[msg.module.outputNumber])
    return false;
    
  ModuleConfig* mod = config->modules[msg.module.outputNumber].get();

  switch (msg.field)
  {
    case ConfigField::FreqLow:      
      mod->freqLow = msg.module.value.u16; 
      return true;

    case ConfigField::FreqHigh:     
      mod->freqHigh = msg.module.value.u16; 
      return true;
      
    case ConfigField::OutputNumber: 
      mod->outputNumber = msg.module.value.i; 
      return true;

    case ConfigField::MinAmpNorm:      
      mod->minAmpNorm = msg.module.value.f;
      return false;

    case ConfigField::IsMuted:
      mod->isMuted = msg.module.value.b;
      return false;

    default:
      break;
  }

  switch (mod->moduleType)
  {
    case MAJORPEAKS:
    {
      auto* mp = static_cast<MajorPeaksConfig*>(mod);
      switch (msg.field)
      {
        case ConfigField::MaxPeaks:         
          mp->maxPeaks = msg.module.value.i; 
          return false;

        case ConfigField::FrequencyMapping: 
          mp->frequencyMapping = msg.module.value.fm; 
          return false;

        default:
          return false;
      }
    }
    case PERCUSSION:
    {
      auto* pc = static_cast<PercussionConfig*>(mod);
      switch (msg.field)
      {
        case ConfigField::FluxThresh:   
          pc->fluxThresh = msg.module.value.f;
          return true;

        case ConfigField::EnergyThresh: 
          pc->energyThresh = msg.module.value.f;
          return true;

        case ConfigField::EntropyThresh:
          pc->entropyThresh = msg.module.value.f;
          return true;

        case ConfigField::WaveType:     
          pc->waveType = msg.module.value.wt;
          return false;

        default:
          return false;
      }
    }
    default:
      return false;
  }
}