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
#include "utils.h"
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
  //! Loads a config and inits the message queue
  bool init();

  //! Atomically gets the current config
  //! NOTE: this should only be called by audio analysis loop b/c it's read only
  std::shared_ptr<const AnalysisConfig> getConfig_r() const { return std::atomic_load(&curConfig); }

  //! Atomically get a mutable pointer to the current config
  auto getConfig_mut() const { return std::atomic_load(&curConfig); }

  //! Adds a message to the update queue for the audio core to use
  bool addMessage(QueueMessage* toSend);
  
  //! Checks if the update queue has pending messages
  bool needsUpdate();

  //! Atomically swaps the current config with the new config
  void updateConfig(std::shared_ptr<AnalysisConfig>& other) { std::atomic_store(&curConfig, other); }

  //! Processes all messages in the update queue
  bool processQueue(bool& sdSaveRequested);

private:
  // Use shared ptr, so that we can replace the settings fast/safe across cores
  std::shared_ptr<AnalysisConfig> curConfig;

  //! Sets the current config to the saved main settings on the SD card
  void loadConfig();

  //! Gets a message from the update queue
  bool getMessage(QueueMessage* toGet);

  // Disable any sort of initialization for the HapticSettings class
  HapticSettings();
  ~HapticSettings() = default;
  HapticSettings(const HapticSettings& other) = delete;
  HapticSettings operator=(const HapticSettings& other) = delete;
  HapticSettings(HapticSettings&&) = delete;
  HapticSettings& operator=(HapticSettings&&) = delete;
};

#endif //HAPTIC_SETTINGS_H