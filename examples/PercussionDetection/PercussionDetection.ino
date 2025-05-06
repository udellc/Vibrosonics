#include "VibrosonicsAPI.h"
#include "PercussionDetection.h"
#include <Arduino.h>

// half-sized FFT window
#define WINDOW_SIZE_BY_2 256  

// VibroSonics API instance
VibrosonicsAPI vapi;

// Spectrogram buffers (keeps two windows each)
Spectrogram rawSpectrogram       = Spectrogram(2);
Spectrogram processedSpectrogram = Spectrogram(2);

// ModuleGroup drives all analysis modules on processedSpectrogram
ModuleGroup modules(&processedSpectrogram);

// Percussion detector thresholds: (loudness, deltaSum, noisiness)
PercussionDetection percussionDetector(500.0f, 400.0f, 0.6f);

// buffer for one FFT window of time-domain samples
float windowData[WINDOW_SIZE_BY_2];

void setup() {
    Serial.begin(115200);
    vapi.init();
    // Restrict detector to 20–2000 Hz (optional)
    percussionDetector.setAnalysisRangeByFreq(20, 2000);
    modules.addModule(&percussionDetector, 20, 2000);
}

void loop() {
    // wait for new audio window
    if (!AudioLab.ready()) return;

    // acquire and transform audio
    vapi.processAudioInput(windowData);
    rawSpectrogram.pushWindow(windowData);
    vapi.noiseFloorCFAR(windowData,
                        WINDOW_SIZE_BY_2,  // length of windowData
                        4,                 // CFAR training cells
                        1,                 // CFAR guard cells
                        1.6f);             // threshold multiplier
    processedSpectrogram.pushWindow(windowData);

    // run all analysis modules (including percussion detector)
    modules.runAnalysis();

    // fetch and print the detection result
    bool hit = percussionDetector.getOutput();
    Serial.printf("Percussion detected: %s\n", hit ? "YES" : "no");

    delay(10);
}
