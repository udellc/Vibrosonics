/***************************************************************
 * FILE: hapticSettings.h
 * 
 * DATE: 02/16/2026
 * 
 * DESCRIPTION: The HapticSettings singleton class is 
 * responsible for holding and parsing audio analysis settings 
 * into JSON structures.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#ifndef HAPTIC_SETTINGS_H
#define HAPTIC_SETTINGS_H

class HapticSettings {
public:
  static HapticSettings& Instance()
  {
    // NOTE: This way of creating a singleton is thread safe and prevents any potential 
    //       issues with core 0 and 1 dangerously interacting with each other
    static HapticSettings self;
    return self;
  };
  // TODO: testing for cross core comms
  void updateCounter() { this->counter++; }
  int readCounter() const { return this->counter; }

private:
  // TODO: testing var for cross core comms
  int counter;

  // Disable any sort of initialization for the HapticSettings class
  HapticSettings();
  ~HapticSettings() = default;
  HapticSettings(const HapticSettings& other) = delete;
  HapticSettings operator=(const HapticSettings& other) = delete;
  HapticSettings(HapticSettings&&) = delete;
  HapticSettings& operator=(HapticSettings&&) = delete;
};

#endif //HAPTIC_SETTINGS_H