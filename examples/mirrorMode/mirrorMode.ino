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
    vapi.addModule(&noisiness);
    vapi.addModule(&maxAmp);
    vapi.addModule(&meanAmp);
    vapi.addModule(&majorPeaks);
    //noisiness.setAnalysisRangeByFreq(300, 1000);
    //maxAmp.setAnalysisRangeByFreq(300, 1000);
    //meanAmp.setAnalysisRangeByFreq(300, 1000);
    //majorPeaks.setAnalysisRangeByFreq(300, 1000);
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
    float* windowData = vapi.spectrogram.getCurrentWindow();
    float freqData[vapi.windowSizeBy2]; 
    float ampData[vapi.windowSizeBy2]; 
    for (int i = 0; i < vapi.windowSizeBy2; i++) {
        freqData[i] = i * vapi.frequencyResolution;
        ampData[i] = windowData[i];
    }
    float** peaksData = peaks->getOutput();
    //vapi.mapAmplitudes(peaksData[MP_AMP], 16, 10000);
    vapi.mapAmplitudes(ampData, vapi.windowSizeBy2, 10000);
    vapi.assignWaves(freqData, ampData, vapi.windowSizeBy2, 0);
    //vapi.assignWaves(peaksData[MP_FREQ], peaksData[MP_AMP], 16, 0);
}
