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
    noisiness.setAnalysisRangeByFreq(300, 1000);
    maxAmp.setAnalysisRangeByFreq(300, 1000);
    meanAmp.setAnalysisRangeByFreq(300, 1000);
    majorPeaks.setAnalysisRangeByFreq(300, 1000);
}

void loop() {
    if (!AudioLab.ready()) {
        return;
    }

    vapi.processInput();
    vapi.analyze();

    if (!isWindowNoise()) {
        synthesizePeaks(&majorPeaks);
    }

    AudioLab.synthesize();
    //AudioLab.printWaves();
}

int isWindowNoise() {
    float energyRatio = maxAmp.getOutput() / meanAmp.getOutput();
    float entropy = noisiness.getOutput();

    if (entropy > 0.96 && energyRatio < 3.5) {
        return true;
    }

    return false;
}

void synthesizePeaks(MajorPeaks* peaks) {
    float** peaksData = peaks->getOutput();
    vapi.mapAmplitudes(peaksData[MP_AMP], 4, 10000);
    vapi.assignWaves(peaksData[MP_FREQ], peaksData[MP_AMP], 4, 0);
}
