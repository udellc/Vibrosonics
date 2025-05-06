
/**
 * @file centroid.ino
 *
 * This example serves a reference point for using centroids with the Vibrosonics API. 
 * It will capture audio, process it using an FFT, and calculate the centroid. 
 */
#include "VibrosonicsAPI.h"

/**
 * BACKGROUND: What is a centroid?
 * A centroid is often referred to as the "center of mass" of audio frequency spectrum (the spectrogram).
 * It is calculated by taking an average of frequencies. A higher centroid usually means the sound 
 * is "brighter" or higher pitched. A lower centroid usually means the sound is lower with bassier sounds. 
 */

// create instance of vibrosonics API 
VibrosonicsAPI vapi = VibrosonicsAPI();

// create windowData that can store one FFT window
float windowData[WINDOW_SIZE_BY_2];
// create spectrogram 
Spectrogram processedSpectrogram = Spectrogram(2);

// create module group and pass spectrogram as a parameter 
ModuleGroup modules = ModuleGroup(&processedSpectrogram);

// create instance of centroid module
Centroid centroid = Centroid();

void setup() {
    Serial.begin(115200);

    // call the API setup function
    vapi.init();

    // add the centroid analysis module
    // this example will run between 20 and 3000 hz but these values can be changed
    modules.addModule(&centroid, 20, 3000);
}

void loop() {
    // skip if new audio window has not been recorded
    if (!AudioLab.ready()) {
    return;
    }

    // process the raw audio signal into frequency domain data
    vapi.processAudioInput(windowData);
    processedSpectrogram.pushWindow(windowData);

    // run analysis on all modules in module group, calls doAnalysis
    modules.runAnalysis();

    // print centroid 
    float centroid = centroid.getOutput();
    // round to 2 decimal places
    Serial.printf("Centroid (Hz): %.2f \n", centroid);

    // print out waves, optional for debugging 
    // AudioLab.printWaves();
}
