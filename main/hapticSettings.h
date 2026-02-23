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
#include <atomic>

class HapticSettings {
public:
  static HapticSettings& Instance()
  {
    //! NOTE: This way of creating a singleton is thread safe and prevents any potential 
    //        issues with core 0 and 1 dangerously interacting with each other. Member vars
    //        still need to be handled carefully, but access to this class is ok.
    static HapticSettings self;
    return self;
  };
  //! Sets the current config to the saved main settings on the SD card
  bool loadConfig();
  
  //! Atomically gets the current config
  //! NOTE: this should only be called by audio analysis loop b/c it's read only
  std::shared_ptr<const AnalysisConfig> getConfig_r() const { return std::atomic_load(&curConfig); }

  //! Atomically get a mutable pointer to the current config
  auto getConfig_mut() const { return std::atomic_load(&curConfig); }

  //! Atomically swaps the current config with the new config
  void updateConfig(std::shared_ptr<AnalysisConfig>& other) { std::atomic_store(&curConfig, other); }

  //! Atomically get whether or not settings have been changed
  bool isDirty() const { return _isDirty.load(std::memory_order_acquire); }

  //! Atomically set the dirty boolean.
  void setIsDirty(const bool NewVal) { _isDirty.store(NewVal, std::memory_order_release); }

private:
  // Use shared ptr, so that we can update the settings fast/safe across cores
  std::shared_ptr<AnalysisConfig> curConfig;
  std::atomic<bool> _isDirty {true};

  // Disable any sort of initialization for the HapticSettings class
  HapticSettings();
  ~HapticSettings() = default;
  HapticSettings(const HapticSettings& other) = delete;
  HapticSettings operator=(const HapticSettings& other) = delete;
  HapticSettings(HapticSettings&&) = delete;
  HapticSettings& operator=(HapticSettings&&) = delete;
};

#endif //HAPTIC_SETTINGS_H