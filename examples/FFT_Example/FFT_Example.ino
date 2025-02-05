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

/*
    MajorPeaks module for analysis
    Defaults to 4 peaks, user can input
    an integer to change this.
*/
MajorPeaks mp = MajorPeaks();

/*
    Run once on ESP32 startup.
    VibrosonicsAPI initializes serial connection
    and AudioLab initialization along with adding
    the MajorPeaks module.
*/
void setup() {
    Serial.begin(9600);
    vapi.init();
    vapi.addModule(&mp);
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

    // Copies samples from AudioLab buffer to vReal, then performs FFT operations.
    // Also pushes data to circular buffer.
    vapi.processInput();
    // Runs doAnalysis with each added module.
    vapi.analyze();

    // Access analyzed data
    float **mp_data = mp.getOutput();
    // Map amplitude data
    //vapi.mapAmplitudes(mp_data[MP_AMP], 4, 250);
    // Map frequency data using linear algorithm 
    // (idk the difference between this and exponential, test on backpack)
    //vapi.mapFrequenciesLinear(mp_data[MP_FREQ], vapi.windowSizeBy2);

    // Use output to synthesize waves
    for (int i=0; i < 4; i++){
        if (mp_data[MP_AMP][i] < 10) {
            continue;
        }
        int freq = interpolateAroundPeak(vapi.circularBuffer.getData(0), round(int(mp_data[MP_FREQ][i] * vapi.frequencyWidth)), SAMPLE_RATE, WINDOW_SIZE);
        //Serial.printf("Found peak: %d %f\n", freq, mp_data[MP_AMP][i]);
        // Mirror mode waves
        AudioLab.dynamicWave(0, freq, mp_data[MP_AMP][i]);
    }

    // map amplitudes so that output waveform isn't clipped
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
