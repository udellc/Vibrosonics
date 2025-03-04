#include "VibrosonicsAPI.h"

VibrosonicsAPI vapi = VibrosonicsAPI();

// set a number of peaks for the major peaks module to find
#define NUM_PEAKS 8

MajorPeaks majorPeaks = MajorPeaks(NUM_PEAKS);

void setup() {
    Serial.begin(115200);

    // call the API setup function
    vapi.init();

    // add the major peaks analysis module
    int lowerFreq = 20;
    int upperFreq = 1800;
    vapi.addModule(&majorPeaks, lowerFreq, upperFreq);
}

void loop() {
    // skip if new audio window has not been recorded
    if (!AudioLab.ready()) {
        return;
    }

    // process the audio signal, filtering noise with CFAR algorithm
    vapi.processInput(4, 4, 1.4);

    // have analysis modules analyze the processed input
    vapi.analyze();

    // create audio waves according the the output of the major peaks module
    synthesizePeaks(&majorPeaks); 

    // synthesize the waves created
    AudioLab.synthesize();

    //AudioLab.printWaves();
}

void synthesizePeaks(MajorPeaks* peaks) {
    float** peaksData = peaks->getOutput();
    vapi.mapAmplitudes(peaksData[MP_AMP], NUM_PEAKS, 10000);
    vapi.assignWaves(peaksData[MP_FREQ], peaksData[MP_AMP], NUM_PEAKS, 0);
}
