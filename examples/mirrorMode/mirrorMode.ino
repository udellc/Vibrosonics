#include "VibrosonicsAPI.h"

VibrosonicsAPI vapi = VibrosonicsAPI();

Noisiness noisiness = Noisiness();
MaxAmplitude maxAmp = MaxAmplitude();
MeanAmplitude meanAmp = MeanAmplitude();

MajorPeaks majorPeaks = MajorPeaks(16);
Grain* grains = vapi.createGrainArray(4, 0, SINE);

void setup() {
    Serial.begin(115200);
    vapi.init();
    int lowerFreq = 300;
    int upperFreq = 4000;
    vapi.addModule(&noisiness, lowerFreq, upperFreq);
    vapi.addModule(&maxAmp, lowerFreq, upperFreq);
    vapi.addModule(&meanAmp, lowerFreq, upperFreq);
    vapi.addModule(&majorPeaks, lowerFreq, upperFreq);
    // Shape the MajorPeaks grains
    vapi.shapeGrainAttack(grains, 4, 1, 1.0, 1.0, 1.0);
    vapi.shapeGrainSustain(grains, 4, 1, 1.0, 0.6);
    vapi.shapeGrainRelease(grains, 4, 1, 1.0, 0.2, 1.0);
}

void loop() {
    if (!AudioLab.ready()) {
        return;
    }

    vapi.processInput(500);
    vapi.analyze();

    if (!isWindowNoise()) {
        synthesizePeaks(&majorPeaks);
    }

    vapi.updateGrains();
    AudioLab.synthesize();
    //AudioLab.printWaves();
}

int isWindowNoise() {
    float energyRatio = maxAmp.getOutput() / meanAmp.getOutput();
    float entropy = noisiness.getOutput();

    if (entropy > 0.85) {
        return true;
    }

    return false;
}

void synthesizePeaks(MajorPeaks* peaks) {
    float* windowData = vapi.getCurrentWindow();
    float freqData[vapi.WINDOW_SIZE_BY_2]; 
    float ampData[vapi.WINDOW_SIZE_BY_2]; 
    for (int i = 0; i < vapi.WINDOW_SIZE_BY_2; i++) {
        freqData[i] = i * vapi.FREQ_RES;
        ampData[i] = windowData[i];
    }
    float** peaksData = peaks->getOutput();
    vapi.mapAmplitudes(peaksData[MP_AMP], 4, 10000);
    //vapi.assignWaves(peaksData[MP_FREQ], peaksData[MP_AMP], 4, 0);
    vapi.triggerGrains(grains, 4, peaksData);
}
