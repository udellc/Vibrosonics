#include "VibrosonicsAPI.h"

VibrosonicsAPI vapi = VibrosonicsAPI();

MajorPeaks mp = MajorPeaks(4);

//Mirror Mode test example.
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
    vapi.mapFrequenciesLinear(mp_data[MP_FREQ], vapi.WINDOW_SIZE_BY2);
    
    for (int i=0; i < 4; i++){
        int freq = vapi.interpolateAroundPeak(round(int(mp_data[MP_FREQ][i] * vapi.frequencyWidth)));
        AudioLab.dynamicWave(0, freq, mp_data[MP_AMP][i]);
    }

    AudioLab.mapAmplitudes(0, 10000);
    AudioLab.synthesize();
    AudioLab.printWaves();
}

