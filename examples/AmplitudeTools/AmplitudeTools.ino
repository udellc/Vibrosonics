#include "VibrosonicsAPI.h"
#include "AmplitudeTools.h"
#include <Arduino.h>

// half‐sized FFT window
#define WINDOW_SIZE_BY_2 256

// frequency bin range
const int LOWER_BIN = 0;
const int UPPER_BIN = WINDOW_SIZE_BY_2;

// API + spectrogram buffer
VibrosonicsAPI vapi;
Spectrogram processedSpectrogram = Spectrogram(2, WINDOW_SIZE_OVERLAP);

// time-domain buffer for one FFT window
float windowData[WINDOW_SIZE_BY_2];

void setup() {
    Serial.begin(115200);
    vapi.init();
}

void loop() {
    // wait for new audio window
    if (!vapi.isAudioLabReady()) {
        return;
    }

    // acquire + FFT + noise floor
    vapi.processAudioInput(windowData);
    vapi.noiseFloorCFAR(windowData,
                        4,                 // CFAR training cells
                        1,                 // CFAR guard cells
                        1.6f);             // threshold multiplier

    // push into buffer so we have current + previous
    processedSpectrogram.pushWindow(windowData);

    // grab pointer to current & previous spectrum
    const float** fftOutput = processedSpectrogram.getWindows();

    // compute amplitude metrics
    float totalAmplitude = AmplitudeTools::total(fftOutput, LOWER_BIN, UPPER_BIN);
    float meanAmplitude  = AmplitudeTools::mean(fftOutput, LOWER_BIN, UPPER_BIN);
    float maxAmplitude   = AmplitudeTools::max(fftOutput, LOWER_BIN, UPPER_BIN);
    float deltaSum       = AmplitudeTools::delta(fftOutput, LOWER_BIN, UPPER_BIN);

    Serial.printf("Total: %f  Mean: %f  Max: %f  ΔSum: %f\n",
                  totalAmplitude, meanAmplitude, maxAmplitude, deltaSum);

    delay(10);
}
