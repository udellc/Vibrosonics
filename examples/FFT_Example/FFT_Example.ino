//Mirror Mode test example.
#include "VibrosonicsAPI.h"

/*
    Instance of the VibroSonicsAPI used to manage:
    -- FFT operations with ArduinoFFT
    -- Audio Spectrum storage with the CircularBuffer structure
    -- Audio input/sampling with AudioLab
    -- AudioPrism module management and synchronization
*/
VibrosonicsAPI vapi = VibrosonicsAPI();

Noisiness noisiness = Noisiness();
MaxAmplitude maxAmp = MaxAmplitude();
MeanAmplitude meanAmp = MeanAmplitude();

/*
    MajorPeaks module for analysis
    Defaults to 4 peaks, user can input
    an integer to change this.
*/
MajorPeaks mp = MajorPeaks();
Grain* mpGrains = vapi.createGrainArray(4, 0, SINE);

/*
    Run once on ESP32 startup.
    VibrosonicsAPI initializes serial connection
    and AudioLab initialization along with adding
    the MajorPeaks module.
*/
void setup() {
    Serial.begin(9600);
    vapi.init();
    //mp.setDebugMode(DEBUG_ENABLE);
    //noisiness.setDebugMode(DEBUG_ENABLE | DEBUG_VERBOSE);
    vapi.addModule(&mp);
    vapi.addModule(&noisiness);
    vapi.addModule(&maxAmp);
    vapi.addModule(&meanAmp);
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
void loop() {
    if (!AudioLab.ready()) {
        return;
    }

    //Serial.printf("!!!Start of Window!!!\n");

    // Copies samples from AudioLab buffer to vReal, then performs FFT operations.
    // Also pushes data to circular buffer.
    vapi.processInput();
    // Runs doAnalysis with each added module.
    vapi.analyze();

    // Access analyzed data
    float **mp_data = mp.getOutput();
    //vapi.triggerGrains(4, mp_data, mpGrains);
    // Map amplitude data
    //vapi.mapAmplitudes(mp_data[MP_AMP], 4, 250);
    // Map frequency data using linear algorithm 
    // (idk the difference between this and exponential, test on backpack)
    //vapi.mapFrequenciesLinear(mp_data[MP_FREQ], vapi.windowSizeBy2);

    // Use output to synthesize waves
    float energyRatio = maxAmp.getOutput() / meanAmp.getOutput();
    //Serial.printf("maxAmp: %f\n", maxAmp.getOutput());
    //Serial.printf("meanAmp: %f\n", meanAmp.getOutput());
    float entropy = noisiness.getOutput();
    int isNoise = 0;
    if (entropy > 0.965 && energyRatio < 3.1) {
        // likely to be isNoise
        //Serial.printf("-- likely to be noise --\n");
        isNoise = 1;
    }
    if (entropy <= 0.965 && entropy > 9.4 && energyRatio < 2.6) {
        // likely to be noise
        //Serial.printf("-- likely to be noise --\n");
        isNoise = 1;
    }

    for (int i=0; i < 4; i++){
        //continue;
        if (isNoise) {
            continue;
        }
        Serial.printf("-- cont --\n");
        Serial.printf("maxEnergyRatio: %f\n", energyRatio);
        Serial.printf("energyRatio%u: %f\n", i, mp_data[MP_AMP][i] / meanAmp.getOutput());
        Serial.printf("noisiness: %f\n", noisiness.getOutput());
        int freq = interpolateAroundPeak(vapi.spectrogram.getWindow(0), round(int(mp_data[MP_FREQ][i] * vapi.frequencyWidth)), SAMPLE_RATE, WINDOW_SIZE);
        // Mirror mode waves
        AudioLab.dynamicWave(0, freq, mp_data[MP_AMP][i]);
    }

    // map amplitudes so that output waveform isn't clipped
    //vapi.updateGrains();
    AudioLab.mapAmplitudes(0, 10000);
    AudioLab.synthesize();
    // For debugging purposes.
    //AudioLab.printWaves();
}

int interpolateAroundPeak(float *data, int indexOfPeak, int sampleRate, int windowSize) {
    float _freqRes = sampleRate * 1.0 / windowSize;
    float prePeak = indexOfPeak == 0 ? 0.0 : data[indexOfPeak - 1];
    float atPeak = data[indexOfPeak];
    float postPeak = indexOfPeak == vapi.windowSizeBy2 ? 0.0 : data[indexOfPeak + 1];
    // summing around the index of maximum amplitude to normalize magnitudeOfChange
    float peakSum = prePeak + atPeak + postPeak;
    // interpolating the direction and magnitude of change, and normalizing from -1.0 to 1.0
    float magnitudeOfChange = ((atPeak + postPeak) - (atPeak + prePeak)) / (peakSum > 0.0 ? peakSum : 1.0);

    // return interpolated frequency
    return int(round((float(indexOfPeak) + magnitudeOfChange) * _freqRes));
}
