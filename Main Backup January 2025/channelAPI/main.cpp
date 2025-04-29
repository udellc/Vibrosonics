#include "channel_API.h"
#include <iostream>
#include <string>
#include <vector>

int main() {
    ChannelAPI api;
    api.init();

    int choice = -1;
    while (choice != 0) {
        std::cout << "\n=== Waveform PNG Generator ===\n";
        std::cout << "Select a waveform to generate:\n";
        std::cout << "1: Sine Wave\n";
        std::cout << "2: Square Wave\n";
        std::cout << "3: Triangle Wave\n";
        std::cout << "4: Sawtooth Wave\n";
        std::cout << "5: Custom Wave (modular custom components)\n";
        std::cout << "0: Exit\n";
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        if (choice == 0) {
            std::cout << "Exiting...\n";
            break;
        }

        float frequency, amplitude;
        std::cout << "Enter frequency in Hz (e.g., 440): ";
        std::cin >> frequency;
        std::cout << "Enter amplitude (0.0 to 1.0, e.g., 1.0): ";
        std::cin >> amplitude;

        int chanId = 0;
        std::string filename;

        switch (choice) {
            case 1:
                chanId = api.createChannel(frequency, amplitude, Waveform::SINE);
                filename = "sine_wave.png";
                break;
            case 2:
                chanId = api.createChannel(frequency, amplitude, Waveform::SQUARE);
                filename = "square_wave.png";
                break;
            case 3:
                chanId = api.createChannel(frequency, amplitude, Waveform::TRIANGLE);
                filename = "triangle_wave.png";
                break;
            case 4:
                chanId = api.createChannel(frequency, amplitude, Waveform::SAWTOOTH);
                filename = "sawtooth_wave.png";
                break;
            case 5: {
                float modFreq, modDepth;
                std::cout << "Enter modulation frequency in Hz (e.g., 2.0): ";
                std::cin >> modFreq;
                std::cout << "Enter modulation depth (0.0 to 1.0, e.g., 0.8): ";
                std::cin >> modDepth;
                chanId = api.createChannel(frequency, amplitude, Waveform::CUSTOM, VibrationMode::STANDARD, modFreq, modDepth);
                filename = "custom_wave.png";
                // Optionally, set modular custom components.
                // For example, here we blend three components: sine, square, and triangle.
                std::vector<CustomComponent> components = {
                    { Waveform::SINE, 0.4f },
                    { Waveform::SQUARE, 0.3f },
                    { Waveform::TRIANGLE, 0.3f }
                };
                api.setCustomComponents(chanId, components);
                break;
            }
            default:
                std::cout << "Invalid choice. Please try again.\n";
                continue;
        }

        if (api.renderWaveformPNG(chanId, filename)) {
            std::cout << "PNG generated: " << filename << "\n";
        } else {
            std::cout << "Failed to generate PNG for the selected waveform.\n";
        }
    }

    return 0;
}
