/***************************************************************
 * FILE: hapticSettings.h
 * 
 * DATE: 02/16/2026
 * 
 * DESCRIPTION: The HapticSettings singleton class is 
 * responsible for holding audio analysis settings.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#ifndef HAPTIC_SETTINGS_H
#define HAPTIC_SETTINGS_H

#include "storage.h"
#include <memory>

class HapticSettings {
public:
  static HapticSettings& Instance()
  {
    // NOTE: This way of creating a singleton is thread safe and prevents any potential 
    //       issues with core 0 and 1 dangerously interacting with each other. Member vars
    //       still need to be handled carefully, but access to this class is ok.
    static HapticSettings self;
    return self;
  };
  //! Sets the current config to the saved main settings on the SD card
  bool loadConfig();

  //! Updates the whole configuration by referencing another object
  bool updateConfig(AnalysisConfig& other);

  //! Returns a read-only reference to the current configurations
  const AnalysisConfig& getConfig() const { return *curConfig; }

private:
  // Use shared ptr, so that we can update the settings fast/safe across cores
  std::shared_ptr<AnalysisConfig> curConfig;

  // Disable any sort of initialization for the HapticSettings class
  HapticSettings();
  ~HapticSettings() = default;
  HapticSettings(const HapticSettings& other) = delete;
  HapticSettings operator=(const HapticSettings& other) = delete;
  HapticSettings(HapticSettings&&) = delete;
  HapticSettings& operator=(HapticSettings&&) = delete;
};

#endif //HAPTIC_SETTINGS_H