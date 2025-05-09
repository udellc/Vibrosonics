#include "VibrosonicsAPI.h"

#define NUM_BASS_PEAKS 1
#define NUM_MID_PEAKS 2

VibrosonicsAPI vapi = VibrosonicsAPI();

float windowData[WINDOW_SIZE_BY_2];

Spectrogram processedSpectrogram = Spectrogram(2);

ModuleGroup modules = ModuleGroup(&processedSpectrogram);
MajorPeaks bassPeaks = MajorPeaks(NUM_BASS_PEAKS);
MajorPeaks midPeaks = MajorPeaks(NUM_MID_PEAKS);
Noisiness noisiness = Noisiness();
MeanAmplitude meanAmp = MeanAmplitude();
PercussionDetection snareDetector = PercussionDetection(200, 150, 0.8);
Grain* snareGrains = vapi.createGrainArray(1, 0, SINE);
Grain& snareGrain = snareGrains[0];


void setup() {
    Serial.begin(115200);
    vapi.init();

    modules.addModule(&bassPeaks, 20, 300);
    modules.addModule(&midPeaks, 300, 2800);
    modules.addModule(&noisiness, 20, 1800);
    modules.addModule(&snareDetector, 2000, 4000);
    modules.addModule(&meanAmp, 2000, 4000);

    vapi.shapeGrainAttack(&snareGrain, 1, 1, 1.0, 1.0, 1.0);
    vapi.shapeGrainSustain(&snareGrain, 1, 1, 1.0, 1.0);
    vapi.shapeGrainRelease(&snareGrain, 1, 2, 1.0, 1.0, 1.0);
}

void loop() {
    if (!AudioLab.ready()) {
        return;
    }

    vapi.processAudioInput(windowData);

    vapi.noiseFloorCFAR(windowData, WINDOW_SIZE_BY_2, 6, 1, 1.6);
    processedSpectrogram.pushWindow(windowData);

    modules.runAnalysis();

    synthesizePeaks(&bassPeaks, 0, NUM_BASS_PEAKS);

    if (noisiness.getOutput() < 0.5) {
        synthesizePeaks(&midPeaks, 1, NUM_MID_PEAKS);
    }

    // // snare detection and triggering
    // if (snareDetector.getOutput() && snareGrain.getGrainState() == READY) {
    //     float mean = meanAmp.getOutput();
    //     vapi.mapAmplitudes(&mean, 1, 75);
    //     float snareAmp = pow(mean, 3.0f);
    //     snareAmp = fmin(snareAmp * 8.0f, 1.0f);

    //     float snareFreq = bassPeaks.getOutput()[MP_FREQ][0] * 2.0f;
    //     if (snareFreq < 100.0f) snareFreq *= 2.0f;
    //     if (snareFreq == 0.0f) snareFreq = 200.0f;

    //     snareGrain.setAttack(snareFreq, snareAmp, snareGrain.getAttackDuration());
    //     snareGrain.setSustain(snareFreq, snareAmp, snareGrain.getSustainDuration());
    //     snareGrain.setRelease(snareFreq, 0.0f, snareGrain.getReleaseDuration());
    // }

    vapi.updateGrains();
    AudioLab.synthesize();
    // AudioLab.printWaves(); 
}

void synthesizePeaks(MajorPeaks* peaks, int channel, int numPeaks) {
    float** peaksData = peaks->getOutput();
    vapi.mapAmplitudes(peaksData[MP_AMP], numPeaks, 10000);
    vapi.assignWaves(peaksData[MP_FREQ], peaksData[MP_AMP], numPeaks, channel);
}
