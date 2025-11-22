/**
 * @file breadSlicer.ino
 *
 * This example serves a reference point for using the breadSlicer with the Vibrosonics API. 
 * It "slices" the frequency spectrum into different bands and outputs the summed amplitudes 
 * in each range. 
 */

#include "VibrosonicsAPI.h"

/**
 * BACKGROUND: What is a breadSlicer?
 * The breadSlicer splits FFT outputs into frequency bands and sums the amplitudes 
 * in each band. It then outputs one value per band, giving a general idea of how 
 * much of the sound is in the low/mid/high range (or whatever you define your bands as).
 *
 * NOTES:
 * Example numbers were set based on general low/mid/high frequency ranges
 * Output is a float array where each element corresponds to a band
 * You must call `setBands()` before using the module
 */

// create instance of vibrosonics API 
VibrosonicsAPI vapi = VibrosonicsAPI();

// create windowData that can store one FFT window
float windowData[WINDOW_SIZE_BY_2];
// create spectrogram 
Spectrogram processedSpectrogram = Spectrogram(2, WINDOW_SIZE_OVERLAP);

// create module group and pass spectrogram as a parameter 
ModuleGroup modules = ModuleGroup(&processedSpectrogram);

// create instance of breadslicer module
BreadSlicer breadSlicer = BreadSlicer();

// define frequency band edges
// ex for low/mid/high: [0-200 Hz], [250-700 Hz], [700-2000 Hz]
int frequencyBands[] = {0, 250, 700, 2000};
const int numBands = 3;

void setup() {
    Serial.begin(115200);
    vapi.init();

    breadSlicer.setBands(frequencyBands, numBands);
    breadSlicer.setWindowSize(WINDOW_SIZE_OVERLAP);

    // add the breadSlicer analysis module
    modules.addModule(&breadSlicer, frequencyBands[0], frequencyBands[numBands]);
}

void loop() {
    if (!vapi.isAudioLabReady()) {
        return;
    }

    vapi.processAudioInput(windowData);
    processedSpectrogram.pushWindow(windowData);

    // run analysis on all modules in module group, calls doAnalysis
    modules.runAnalysis();

    // print breadSlicer bands
    float* amps = breadSlicer.getOutput();
    Serial.printf("BreadSlicer Output: \n");
    for (int i = 0; i < numBands; i++) {
        Serial.printf("Band %d: %.2f\n", i, amps[i]);
    }
    Serial.println();

    // print out waves, optional for debugging 
    // AudioLab.printWaves();
}