#include "VibrosonicsAPI.h"

VibrosonicsAPI vapi = VibrosonicsAPI();

Noisiness noisiness = Noisiness();
MaxAmplitude maxAmp = MaxAmplitude();
MeanAmplitude meanAmp = MeanAmplitude();

MajorPeaks majorPeaks = MajorPeaks();

void setup() {
    Serial.begin(115200);
    vapi.init();

    int lower_freq = 20, upper_freq = 1800;
    vapi.addModule(&noisiness, lower_freq, upper_freq);
    vapi.addModule(&maxAmp, lower_freq, upper_freq);
    vapi.addModule(&meanAmp, lower_freq, upper_freq);
    vapi.addModule(&majorPeaks, lower_freq, upper_freq);
}

void loop() {
    if (!AudioLab.ready()) {
        return;
    }

    Serial.printf("New Window:\n");

    // process the audio signal with no noise filtering
    vapi.processInput();

    // have the analysis modules analyze the processed signal
    vapi.analyze();

    // find the energy ratio and entropy (measure of uniformity) based on
    // analysis module output
    float energyRatio = maxAmp.getOutput() / meanAmp.getOutput();
    float entropy = noisiness.getOutput();

    // print the most significant peaks, energy ratio, and entropy of each window
    float** majorPeaksData = majorPeaks.getOutput();
    for (int i = 0; i < 4; i++) {
        float freq = majorPeaksData[MP_FREQ][i];
        float amp = majorPeaksData[MP_AMP][i];

        Serial.printf("  (%f, %f)\n", freq, amp);
    }

    Serial.printf("  %f\n", energyRatio);
    Serial.printf("  %f\n", entropy);
}
