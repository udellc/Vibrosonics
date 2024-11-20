#include "VibrosonicsAPI.h"

VibrosonicsAPI vapi = VibrosonicsAPI();

MajorPeaks mp = MajorPeaks(4);


void setup() {
    vapi.init();
    vapi.addModule(&mp);
    mp.setWindowSize(WINDOW_SIZE);
    mp.setSampleRate(SAMPLE_RATE);
}

void loop() {
    if (!AudioLab.ready()) {
        return;
    }

    vapi.processInput();
    vapi.analyze();

    float **mp_data = mp.getOutput();
    vapi.mapAmplitudes(mp_data[MP_AMP], 4, 250);
    vapi.mapFrequenciesLinear(mp_data[MP_FREQ], WINDOW_SIZE_BY2);
    
    AudioLab.mapAmplitudes(0, 10000);
    AudioLab.synthesize();
    AudioLab.printWaves();
}