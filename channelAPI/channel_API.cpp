#include "channel_API.h"
#include <fstream>
#include <sstream>
#include <vector>

// For PNG writing using stb_image_write.
// Make sure to have "stb_image_write.h" in your project directory.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// --------------------- Existing Methods ---------------------

void ChannelAPI::init() {
    std::cout << "ChannelAPI Initialized\n";
}

int ChannelAPI::createChannel(float frequency, float amplitude, Waveform waveform,
                              VibrationMode mode, float modulationFrequency, float modulationDepth) {
    int channelId = nextChannelId++;
    // Initialize customComponents as empty.
    channels[channelId] = { channelId, frequency, amplitude, waveform, false,
                            mode, modulationFrequency, modulationDepth, {} };
    std::cout << "Channel " << channelId << " created with "
              << "Frequency: " << frequency << " Hz, "
              << "Amplitude: " << amplitude << ", "
              << "Waveform: " << static_cast<int>(waveform)
              << ", Mode: " << static_cast<int>(mode)
              << ", Modulation Frequency: " << modulationFrequency
              << ", Modulation Depth: " << modulationDepth << "\n";
    return channelId;
}

bool ChannelAPI::updateChannel(int channelId, float frequency, float amplitude, Waveform waveform,
                               VibrationMode mode, float modulationFrequency, float modulationDepth) {
    if (channels.find(channelId) == channels.end()) {
        return false;
    }
    // Preserve the current active state and custom components.
    Channel &ch = channels[channelId];
    ch.frequency = frequency;
    ch.amplitude = amplitude;
    ch.waveform = waveform;
    ch.mode = mode;
    ch.modulationFrequency = modulationFrequency;
    ch.modulationDepth = modulationDepth;
    std::cout << "Channel " << channelId << " updated\n";
    return true;
}

bool ChannelAPI::activateChannel(int channelId) {
    if (channels.find(channelId) == channels.end())
        return false;
    channels[channelId].active = true;
    std::cout << "Channel " << channelId << " activated\n";
    return true;
}

bool ChannelAPI::deactivateChannel(int channelId) {
    if (channels.find(channelId) == channels.end())
        return false;
    channels[channelId].active = false;
    std::cout << "Channel " << channelId << " deactivated\n";
    return true;
}

bool ChannelAPI::deleteChannel(int channelId) {
    if (channels.erase(channelId)) {
        std::cout << "Channel " << channelId << " deleted\n";
        return true;
    }
    return false;
}

std::vector<Channel> ChannelAPI::getActiveChannels() {
    std::vector<Channel> activeChannels;
    for (const auto &pair : channels) {
        if (pair.second.active)
            activeChannels.push_back(pair.second);
    }
    return activeChannels;
}

Channel* ChannelAPI::getChannel(int channelId) {
    if (channels.find(channelId) == channels.end())
        return nullptr;
    return &channels[channelId];
}

bool ChannelAPI::augmentChannel(int channelId, VibrationMode mode, float modulationFrequency, float modulationDepth) {
    if (channels.find(channelId) == channels.end())
        return false;
    Channel &ch = channels[channelId];
    ch.mode = mode;
    ch.modulationFrequency = modulationFrequency;
    ch.modulationDepth = modulationDepth;
    std::cout << "Channel " << channelId << " augmented with mode " << static_cast<int>(mode)
              << ", modulation frequency: " << modulationFrequency
              << ", modulation depth: " << modulationDepth << "\n";
    return true;
}

bool ChannelAPI::setCustomComponents(int channelId, const std::vector<CustomComponent>& components) {
    if (channels.find(channelId) == channels.end())
        return false;
    channels[channelId].customComponents = components;
    std::cout << "Custom components set for channel " << channelId << "\n";
    return true;
}

// ------------------ Helper Function ------------------

// Computes a sample value at time t for the given channel.
// For the CUSTOM waveform, if customComponents are defined,
// we compute a weighted sum of the specified components. Otherwise, we fall back to a default blend.
// Updated computeSample function with dynamic modulation for CUSTOM waveform.
static float computeSample(const Channel& ch, float t) {
    switch(ch.waveform) {
        case Waveform::SINE:
            return ch.amplitude * sin(2 * M_PI * ch.frequency * t);
        case Waveform::SQUARE:
            return ch.amplitude * (sin(2 * M_PI * ch.frequency * t) >= 0 ? 1 : -1);
        case Waveform::TRIANGLE:
            return ch.amplitude * ((2.0f / M_PI) * asin(sin(2 * M_PI * ch.frequency * t)));
        case Waveform::SAWTOOTH:
            return ch.amplitude * (2 * (t * ch.frequency - floor(t * ch.frequency + 0.5f)));
        case Waveform::CUSTOM: {
            if (!ch.customComponents.empty()) {
                float sampleSum = 0.0f;
                float totalWeight = 0.0f;
                // Compute a modulation phase offset (dynamic over time).
                float modPhase = ch.modulationDepth * sin(2 * M_PI * ch.modulationFrequency * t);
                for (const auto& comp : ch.customComponents) {
                    totalWeight += comp.weight;
                    float baseSample = 0.0f;
                    // Apply the modulation phase offset to each component.
                    switch(comp.waveform) {
                        case Waveform::SINE:
                            baseSample = sin(2 * M_PI * ch.frequency * t + modPhase);
                            break;
                        case Waveform::SQUARE:
                            baseSample = (sin(2 * M_PI * ch.frequency * t + modPhase) >= 0 ? 1 : -1);
                            break;
                        case Waveform::TRIANGLE:
                            baseSample = (2.0f / M_PI) * asin(sin(2 * M_PI * ch.frequency * t + modPhase));
                            break;
                        case Waveform::SAWTOOTH:
                            // For sawtooth, shift time by the equivalent phase offset.
                            baseSample = 2 * (((t + modPhase/(2*M_PI)) * ch.frequency) - floor(((t + modPhase/(2*M_PI)) * ch.frequency) + 0.5f));
                            break;
                        case Waveform::CUSTOM:
                            // Avoid recursion; fallback to sine with modulation.
                            baseSample = sin(2 * M_PI * ch.frequency * t + modPhase);
                            break;
                    }
                    sampleSum += comp.weight * baseSample;
                }
                if (totalWeight > 0)
                    return ch.amplitude * (sampleSum / totalWeight);
                else
                    return 0.0f;
            } else {
                // Fallback: blend of sine and triangle using modulation.
                float lfoFreq = (ch.modulationFrequency > 0) ? ch.modulationFrequency : 0.5f;
                float lfo = (sin(2 * M_PI * lfoFreq * t) + 1.0f) / 2.0f;
                float sineComponent = sin(2 * M_PI * ch.frequency * t);
                float triangleComponent = (2.0f / M_PI) * asin(sin(2 * M_PI * ch.frequency * t));
                float blend = lfo * ch.modulationDepth + (1.0f - ch.modulationDepth);
                return ch.amplitude * (blend * sineComponent + (1.0f - blend) * triangleComponent);
            }
        }
        default:
            return 0.0f;
    }
}


// ------------------ Rendering as PPM ------------------

bool ChannelAPI::renderWaveform(int channelId, const std::string &filename, int width, int height) {
    if (channels.find(channelId) == channels.end()) {
        std::cerr << "Channel with id " << channelId << " not found.\n";
        return false;
    }
    const Channel &ch = channels[channelId];

    // Render two full periods.
    float totalTime = (ch.frequency > 0) ? (2.0f / ch.frequency) : 1.0f;
    std::vector<unsigned char> image(width * height * 3, 255);

    int prev_y = -1;
    for (int x = 0; x < width; ++x) {
        float t = (static_cast<float>(x) / width) * totalTime;
        float sample = computeSample(ch, t);

        float normalized = (ch.amplitude != 0) ? (sample / ch.amplitude) : 0;
        int y = static_cast<int>((height / 2) - normalized * (height / 2));
        if (y < 0) y = 0;
        if (y >= height) y = height - 1;

        // Draw a connecting line from previous sample to current.
        if (prev_y != -1) {
            int y_start = (prev_y < y) ? prev_y : y;
            int y_end = (prev_y > y) ? prev_y : y;
            for (int yy = y_start; yy <= y_end; ++yy) {
                int idx = (yy * width + x) * 3;
                image[idx] = 0;
                image[idx+1] = 0;
                image[idx+2] = 0;
            }
        }
        // Draw a vertical line for thickness.
        for (int dy = -1; dy <= 1; ++dy) {
            int yy = y + dy;
            if (yy >= 0 && yy < height) {
                int idx = (yy * width + x) * 3;
                image[idx] = 0;
                image[idx+1] = 0;
                image[idx+2] = 0;
            }
        }
        prev_y = y;
    }

    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        std::cerr << "Could not open file " << filename << " for writing.\n";
        return false;
    }
    ofs << "P3\n" << width << " " << height << "\n255\n";
    for (int i = 0; i < width * height; ++i) {
        int idx = i * 3;
        ofs << static_cast<int>(image[idx]) << " "
            << static_cast<int>(image[idx+1]) << " "
            << static_cast<int>(image[idx+2]) << "\n";
    }
    ofs.close();
    std::cout << "Waveform rendered successfully to " << filename << "\n";
    return true;
}

// ------------------ Rendering as PNG ------------------

bool ChannelAPI::renderWaveformPNG(int channelId, const std::string &filename, int width, int height) {
    if (channels.find(channelId) == channels.end()) {
        std::cerr << "Channel with id " << channelId << " not found.\n";
        return false;
    }
    const Channel &ch = channels[channelId];
    float totalTime = (ch.frequency > 0) ? (2.0f / ch.frequency) : 1.0f;
    std::vector<unsigned char> image(width * height * 3, 255);

    int prev_y = -1;
    for (int x = 0; x < width; ++x) {
        float t = (static_cast<float>(x) / width) * totalTime;
        float sample = computeSample(ch, t);

        float normalized = (ch.amplitude != 0) ? (sample / ch.amplitude) : 0;
        int y = static_cast<int>((height / 2) - normalized * (height / 2));
        if (y < 0) y = 0;
        if (y >= height) y = height - 1;

        if (prev_y != -1) {
            int y_start = (prev_y < y) ? prev_y : y;
            int y_end = (prev_y > y) ? prev_y : y;
            for (int yy = y_start; yy <= y_end; ++yy) {
                int idx = (yy * width + x) * 3;
                image[idx] = 0;
                image[idx+1] = 0;
                image[idx+2] = 0;
            }
        }
        for (int dy = -1; dy <= 1; ++dy) {
            int yy = y + dy;
            if (yy >= 0 && yy < height) {
                int idx = (yy * width + x) * 3;
                image[idx] = 0;
                image[idx+1] = 0;
                image[idx+2] = 0;
            }
        }
        prev_y = y;
    }
    int stride_in_bytes = width * 3;
    if (!stbi_write_png(filename.c_str(), width, height, 3, image.data(), stride_in_bytes)) {
        std::cerr << "Failed to write PNG file " << filename << "\n";
        return false;
    }
    std::cout << "Waveform rendered successfully to " << filename << "\n";
    return true;
}
