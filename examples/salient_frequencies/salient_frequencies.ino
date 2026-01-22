/**
 * @file salient_frequencies.ino
 *
 * This example serves a reference point for using the SalientFreqs module of audioPrism.
 * SalientFreqs serves to identify the frequency bins which experienced the biggest change 
 * between the current and previous window captures.
 */
#include <VibrosonicsAPI.h>

/**
 * Instance of the Vibrosonics API. Core to all operations including:
 * -- FFT operations with Fast4ier
 * -- Audio spectrum storage with the spectrogram
 * -- Audio input and synthesis through AudioLab
 * -- AudioPrism module management and analysis
 */
VibrosonicsAPI vapi = VibrosonicsAPI();

// DATA
// Array that stores the AudioLab input buffer time domain data
// After FFT processing it becomes the current window's spectrogram data
float windowData[WINDOW_SIZE_BY_2];

// Stores 2 windows of processed data
// Most of the time you will be using processed data for analysis
Spectrogram processedSpectrogram = Spectrogram(2, WINDOW_SIZE_OVERLAP);

// MODULES
// Create the ModuleGroup to store initialized modules
ModuleGroup modules = ModuleGroup(&processedSpectrogram);

/**
 * Here is where we declare the analysis modules to use
 * In our case, I create deltas to store our instance of SalientFreqs
 * The base number found will 3, if you want to find a specific number
 * use SalientFreqs(n) where n is the number to find
 */
SalientFreqs deltas = SalientFreqs();

/**
 * Setup is a necessary component of any arduino ino to be used in analysis
 * We setup the serial output at a baud rate of 115200 since that is what the ESP32 uses
 * vapi.init initializes the vibrosonicsapi to begin our analysis
 * we then add our SalientFreqs module deltas into the analysis of the api
 */
void setup() {
    Serial.begin(115200);
    vapi.init();
    deltas.setWindowSize(WINDOW_SIZE_OVERLAP);
    modules.addModule(&deltas, 20, 3000);
}

/**
 * This main loop is the iteration that is constantly occuring when our arduino is running
 * All of the main anylysis is here
 */
void loop() {
    // vapi.isAudioLabReady() returns true when synthesis should occur/input buffer fills 
    // (this returns true at (SAMPLE_RATE / WINDOW_SIZE) times per second)
    if (!vapi.isAudioLabReady()) {
        return;
    }
    // processInput runs an FFT on the complex audio data captured from audioLab
    // this turns it into usable frequency-amplitude data
    vapi.processAudioInput(windowData);

    processedSpectrogram.pushWindow(windowData);

    // analyze will pass the audio data to every module you loaded to run in vapi
    modules.runAnalysis();

    // getOutput is used for any module to retrieve the analyzed data of the module
    // in the case, the module will return a 1D array of indexes
    // The base number of index the module will find is 3
    int* salient = deltas.getOutput();

    // salient frequencies module returns indexs so we need the audio windows to see what the change is
    float* previous_window = processedSpectrogram.getPreviousWindow();
    float* current_window = processedSpectrogram.getCurrentWindow();

    // We print out the two amplitude changes as well as the frequency bin in which this occured
    Serial.printf("Salient Frequencies: ");
    for (int i=0; i < 3; i++){
        // each bin of a window corresponds to a frequency range given by FREQ_RES
        Serial.printf("Freq: %iHz Delta: %f-%f ", FREQ_RES * salient[i], previous_window[salient[i]], current_window[salient[i]]);
    }
    Serial.printf("\n");

}
