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
}

#endif