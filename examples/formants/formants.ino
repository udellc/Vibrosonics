/**
 * @file formants.ino
 *
 * This example serves a reference point for using the Formants module of audioPrism.
 * Formants are identifiable peaks of the frequency-amplitude graph that appear within
 * certain vowel sounds. Formants seeks to detect these by find the local maxima of a
 * window and matching that to encoded charts of vowel formants to determin if a
 * vowel is present
 */
#include <VibrosonicsAPI.h>

/**
 * Decloration of the Vibrosonics API that holds the transformed audio data and keeps
 * track of each analysis module used
 */
VibrosonicsAPI vapi = VibrosonicsAPI();

/**
 * Decloration of our Formants module to be used as vocals
 */
Formants vocals = Formants();

/**
 * the Setup loop is used to initialize all the necessary peices before the program
 * starts execution.
 * Serial.begin sets up the serial output which is used for printed
 * vapi.init initializes the api to be ready to revieve data
 * vapi.addModule is used to track our Formants module within the api
 */
void setup() {
    Serial.begin(115200);
    vapi.init();
    vapi.addModule(&vocals);
}

/**
 * The main loop of the program.
 * Waits for AudioLab to revieve a full buffer of audio.
 * Process the current buffer.
 * Analyses the processed buffer with initialized modules.
 * Do what you want with that analyzed data.
 */
void loop() {
    // AudioLab.ready() returns true when synthesis should occur/input buffer fills 
    // (this returns true at (SAMPLE_RATE / WINDOW_SIZE) times per second)
    if (!AudioLab.ready()) {
        return;
    }

    // Perform FFT on the input buffer to transform the complex audio data into usable
    // freqency-amplitude data
    vapi.processInput();

    // Run all the initialized analysis modules on the transformed audio data
    vapi.analyze();
    
    // Use getOutput to recieve the analysed output of any module we initialized
    // In this case, Formants will return its best guess at a vowel 
    // ('a', 'e', 'i', 'o', 'u', or '-')
    Serial.printf("%c\n", vocals.getOutput());
    
}