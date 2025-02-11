#include "VibrosonicsAPI.h"

VibrosonicsAPI vapi = VibrosonicsAPI();

Noisiness noisiness = Noisiness();
MaxAmplitude maxAmp = MaxAmplitude();
MeanAmplitude meanAmp = MeanAmplitude();

MajorPeaks majorPeaks = MajorPeaks();
Grain* grains = vapi.createGrainArray(4, 0, SINE);

void setup() {
    Serial.begin(9600);
    vapi.init();
    vapi.addModule(&noisiness);
    vapi.addModule(&maxAmp);
    vapi.addModule(&meanAmp);
    vapi.addModule(&majorPeaks);
}

void loop() {
    if (!AudioLab.ready()) {
        return;
    }

    Serial.printf("New Window:\n");

    vapi.processInput();
    vapi.analyze();

    float energyRatio = maxAmp.getOutput() / meanAmp.getOutput();
    float entropy = noisiness.getOutput();

    float** majorPeaksData = majorPeaks.getOutput();
    for (int i = 0; i < 4; i++) {
        float freq = majorPeaksData[MP_FREQ][i];
        float amp = majorPeaksData[MP_AMP][i];

        Serial.printf("  (%f, %f)\n", freq, amp);
    }

    Serial.printf("  %f\n", energyRatio);
    Serial.printf("  %f\n", entropy);
}
