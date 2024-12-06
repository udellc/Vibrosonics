//Mirror Mode test example.
#include "VibrosonicsAPI.h"

/*
    Run once on ESP32 startup.
    VibrosonicsAPI initializes serial connection
    and AudioLab initialization along with adding
    the MajorPeaks module.
*/
void setup()
{
   init();
}

/*
    Main loop for audio input, synthesis, and output.
    Waits for AudioLab buffer to fill before perfroming
    FFT operations and module analysis. In this case,
    the only module applied is MajorPeaks.
    MajorPeak frequencies and amplitudes are mapped
    before AudioLab waves are generated.
    Lastly, AudioLab outputs waves with synthesize().
*/
void loop()
{
    Serial.printf("---------------------------------------------\n");
    Serial.printf("before audiolab ready loop\n");
    if (!AudioLab.ready()) {
        return;
    }
    Serial.printf("after audiolab ready loop\n");

    // Copies samples from AudioLab buffer to vReal, then performs FFT operations.
    // Also pushes data to circular buffer.
    Serial.printf("---------------------------------------------\n");
    Serial.printf("before process input\n");
    performFFT();
    Serial.printf("after process input\n");
    // Runs doAnalysis with each added module.
    Serial.printf("---------------------------------------------\n");
    Serial.printf("before analyze\n");
    analyze();
    Serial.printf("after analyze\n");

    // Access analyzed data
    float **mp_data = mp.getOutput();
    // Map amplitude data
    //vapi.mapAmplitudes(mp_data[MP_AMP], 4, 250);
    // Map frequency data using linear algorithm
    // (idk the difference between this and exponential, test on backpack)
    //vapi.mapFrequenciesLinear(mp_data[MP_FREQ], vapi.WINDOW_SIZE_BY2);

    // Use output to synthesize waves
    for (int i=0; i < 4; i++){
        int freq = interpolateAroundPeak(round(int(mp_data[MP_FREQ][i] * frequencyWidth)));
        // Mirror mode waves
        createDynamicWave(0, freq, mp_data[MP_AMP][i]);
    }
    //AudioLab.dynamicWave(0, 100, 200);

    AudioLab.synthesize();
    // For debugging purposes.
    AudioLab.printWaves();
    Serial.printf("TEST\n");
}

