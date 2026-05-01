/***************************************************************
 * FILE: utils.h
 * 
 * DATE: 2/19/2026
 * 
 * DESCRIPTION: The Utils namespace includes global helper
 * functions that are used by multiple classes or namespaces.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#ifndef UTILS_H
#define UTILS_H

#include "storage.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <cstdint>

// Structures for messaging based communication between cores
enum class QueueMsgId : uint
{
  EditGlobal = 0u,
  EditModule,
  UpdateAll,
  CreateModule,
  DeleteModule
};

struct EditGlobalData
{
  union {
    int i;
    uint16_t u16;
    float f;
  } value;
};

struct EditModuleData
{
  int index;

  union {
    int i;
    uint16_t u16;
    float f;
    FrequencyMapping fm;
    WaveType wt;
    bool b;
  } value;
};

struct QueueMessage
{
  QueueMsgId id;
  ConfigField field;

  union {
    EditGlobalData global;
    EditModuleData module;
  };
};

namespace Utils
{
  //! Helper function to parse the global settings Json object into an analysis module
  //! NOTE: This function assumes that the global object is pulled using the schema from
  //        WebApp/data json files
  void populateGlobalSettings(JsonObject& global, AnalysisConfig* config);

  //! Top level helper function to parse modules list Json object into an analysis module
  //! NOTE: This function assumes that the modulesList object is pulled using the schema from
  //        WebApp/data json files
  void populateModulesList(JsonArray& modulesList, AnalysisConfig* config);

  //! Helper function to package analysis config into JsonObject
  void packageGlobalSettings(JsonObject& global, AnalysisConfig* config);

  //! Helper function to package analysis modules into JsonArray
  void packageModulesList(JsonArray& modulesList, AnalysisConfig* config);

  //! Creates an instance of ModulePtr based on the module type
  inline ModulePtr createModule(const ModuleType Type);

  //! Creates a message for the update queue
  void createMessage(const QueueMsgId id, const JsonObject& payload, QueueMessage& msg);

  //! Edits the global config data using the message as which field and value
  void applyGlobalEdit(AnalysisConfig* config, const QueueMessage& msg);

  //! Edits the module config data using the message as which field, index, and value
  bool applyModuleEdit(AnalysisConfig* config, const QueueMessage& msg);
}

#endif