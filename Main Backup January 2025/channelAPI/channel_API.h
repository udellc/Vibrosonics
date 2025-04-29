#ifndef CHANNEL_API_H
#define CHANNEL_API_H

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <cmath>

// Enum for basic waveform types
enum class Waveform { SINE, SQUARE, TRIANGLE, SAWTOOTH, CUSTOM };

// New enum for vibration (or augmentation) modes
enum class VibrationMode { STANDARD, MODULATED, RAMPED };

/// Structure representing a custom component.
/// Each component consists of a waveform and a weight.
struct CustomComponent {
    Waveform waveform;
    float weight;
};

/// Structure to define a channel, including augmentation parameters.
struct Channel {
    int id;                     // Unique Channel ID
    float frequency;            // Frequency in Hz
    float amplitude;            // Amplitude (0.0 - 1.0)
    Waveform waveform;          // Basic waveform type
    bool active;                // Channel active status

    // Augmentation Fields
    VibrationMode mode;         // Vibration mode (STANDARD, MODULATED, RAMPED, etc.)
    float modulationFrequency;  // For MODULATED mode: modulation frequency (Hz)
    float modulationDepth;      // For MODULATED mode: deviation from the base frequency

    // NEW: Custom components for a modular custom waveform.
    // If empty, the default CUSTOM behavior is used.
    std::vector<CustomComponent> customComponents;
};

class ChannelAPI {
private:
    std::map<int, Channel> channels;   // Container for channels
    int nextChannelId = 1;             // Auto-incremented ID for new channels

public:
    // Initializes the Channel API
    void init();

    /**
     * Creates a new channel with basic parameters and optional augmentation.
     */
    int createChannel(float frequency, float amplitude, Waveform waveform,
                      VibrationMode mode = VibrationMode::STANDARD,
                      float modulationFrequency = 0.0f, float modulationDepth = 0.0f);
    
    /**
     * Updates an existing channel.
     */
    bool updateChannel(int channelId, float frequency, float amplitude, Waveform waveform,
                       VibrationMode mode, float modulationFrequency, float modulationDepth);
    
    // Activates a channel
    bool activateChannel(int channelId);
    
    // Deactivates a channel
    bool deactivateChannel(int channelId);
    
    // Deletes a channel
    bool deleteChannel(int channelId);
    
    // Retrieves all active channels
    std::vector<Channel> getActiveChannels();
    
    // Retrieves a specific channel by ID
    Channel* getChannel(int channelId);

    /**
     * Updates only the augmentation parameters on an existing channel.
     */
    bool augmentChannel(int channelId, VibrationMode mode, float modulationFrequency, float modulationDepth);

    /**
     * Sets custom components for a channel (only applicable if the channel's waveform is CUSTOM).
     * This allows a modular mix of different waveforms.
     */
    bool setCustomComponents(int channelId, const std::vector<CustomComponent>& components);

    /**
     * Renders the waveform for a channel as a PPM image.
     * @param channelId The channel to render.
     * @param filename Output file (e.g., "wave.ppm").
     * @param width Width of the image (default 800).
     * @param height Height of the image (default 400).
     * @return true if successful.
     */
    bool renderWaveform(int channelId, const std::string &filename, int width = 800, int height = 400);

    /**
     * Renders the waveform for a channel as a PNG image.
     * @param channelId The channel to render.
     * @param filename Output PNG file (e.g., "wave.png").
     * @param width Width of the image (default 800).
     * @param height Height of the image (default 400).
     * @return true if successful.
     */
    bool renderWaveformPNG(int channelId, const std::string &filename, int width = 800, int height = 400);
};

#endif // CHANNEL_API_H
