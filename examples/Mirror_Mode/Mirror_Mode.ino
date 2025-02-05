/**
* @file Vibrosonics.ino
*
* @mainpage Vibrosonics Mirror Mode Example
*
* @section description 
* An example of how mirror mode works using the vibrosonics API 
*
*/

#include "VibrosonicsAPI.h"

/**
 * NUM_BASS_PEAKS and NUM_MID_PEAKS are used to determine the number of output 
 * voices to generate for each haptic driver. These are set to 1 because 
 * outputting multiple voices on a single driver creates roughness and obscures 
 * tonality. 
 * -- Note: NUM_MID_PEAKS actually generates twice as many waves as listed, but 
 * they are gauranteed to be in a perfect octave relationship, preventing output 
 * roughness.
*/
#define NUM_BASS_PEAKS 1
#define NUM_MID_PEAKS 1

/**
 * vapi is an instance of the VibrosonicsAPI used to manage:
 * -- Audio input / sampling with AudioLab
 * -- Audio output / synthesis with AudioLab and the Grain structure
 * -- FFT computation with ArduinoFFT
 * -- Audio Spectrum storage with the CircularBuffer structure
 * -- AudioPrism module synchronization
*/
VibrosonicsAPI vapi = VibrosonicsAPI();

/**
 * create major peaks module and reserve grains for bass
 * MajorPeaks(int numPeaks)
 *  $1 - numPeaks tells the module how many peaks (freq, amp) to find
*/
MajorPeaks bass_peaks = MajorPeaks(NUM_BASS_PEAKS);
Grain* bass_grains = vapi.createGrainArray(NUM_BASS_PEAKS, 0, SINE);

// create major peaks module and reserve grains for mid range
MajorPeaks mid_peaks = MajorPeaks(NUM_MID_PEAKS);
Grain* mid_grains = vapi.createGrainArray(NUM_MID_PEAKS, 1, SINE);
Grain* mid_low_grains = vapi.createGrainArray(NUM_MID_PEAKS, 1, SINE);

// create a Noisiness module to reject percussion
Noisiness noisiness = Noisiness();

// create a MeanAmplitude module to define snare grain amplitude
MeanAmplitude meanAmp = MeanAmplitude();

/**
 * PercussionDetection(float loudness_threshold, float delta_threshold, float noise_threshold)
 *  $1 - loudness_threshold is the minimum amplitude required for a window to be considered percussive
 *  $2 - delta threshold is the minimum change in amplitude required for a window to be considered percussive
 *  $3 - noise threshold is the minimum noisiness required for a window to be considered percussive (0-1)
*/
PercussionDetection snare_detector = PercussionDetection(200, 150, 0.8);
Grain snare_grain = *vapi.createGrain(0, SINE);

float mid_last_freq = 0;

/**
 * setup() runs once at the beginning of execution.
 * This is where Analysis Modules and grains are configured
 *
 * bass_peaks is configured to find peaks between 0-100 Hz
 * bass_grains are configured to output on channel 0 and skip their attack phase
 *
 * mid_peaks is configured to find peaks between 300-1000 Hz
 * mid_grains are configured to output on channel 1 and skip their attack phase
 *
 * the snare_detector is configured to analyze 1500-3000 Hz
*/
void setup() {
    Serial.begin(9600);
    vapi.init();

    /**
     * initialize low end analysis
     * AnalysisModule::setAnalysisRangeByFreq(int lowerFreq, int upperFreq)
     *  $1 - lowerFreq is the lower frequency bound of the analysis module
     *  $2 - upperFreq is the upper frequency bound of the analysis module
    */
    bass_peaks.setAnalysisRangeByFreq(0, 100);

    // Vibrosonics::addModule(AnalysisModule* module)
    //  $1 - module is the address of the module to be added
    vapi.addModule(&bass_peaks);

    // set up midrange peaks analysis
    vapi.addModule(&mid_peaks);
    noisiness.setAnalysisRangeByFreq(300, 800);
    mid_peaks.setAnalysisRangeByFreq(300, 800);

    // set up snare detector and grain
    vapi.addModule(&snare_detector);
    snare_detector.setAnalysisRangeByFreq(2000, 4000);

    // set up meanAmp (used for snare_grain amplitude)
    vapi.addModule(&meanAmp);
    meanAmp.setAnalysisRangeByFreq(2000, 4000);

    Serial.printf("SETUP COMPLETE\n");
}

/**
 * MAIN RUNNING LOOP
 * If AudioLab is ready, the following processing steps take place:
 *
 * 1. vapi.processInput performs FFT on AudioLab's input buffer and stores the
 * result in VibrosonicsAPI's CircularBuffer.
 * 
 * 2. vapi.analyze prompts all AudioPrism modules to run their analysis methods
 * simulataneously.
 *
 * 3. Bass is translated by finding the BASS_NUM_PEAKS highest peak frequencies 
 * in the 0 - 100 Hz range and using their frequencies and amplitudes to
 * conditionally trigger a set of grains if the new amplitude is louder than
 * the grain's current amplitude.
 * 
 * 4. Mid-Range (300 Hz - 800 Hz) is translated and synthesized similarly to the
 * bass, but with some additional transposition steps and using two grain sets.
 * 
 * 5. Snare (detected in 2000 Hz - 4000 Hz) is conditionally analyzed and
 * triggered based on the PercussionDetection module snare_detector. When a
 * snare hit is detected, an output frequency and amplitude are calculated and
 * used to trigger a grain reserved for snare output.
*/
void loop() {

    // AudioLab.ready() returns True when AudioLab's input buffer is full.
    // This gaurd protects analysis and synthesis logic, which is run only when
    // a new window of audio is ready to be processed.
    if (!AudioLab.ready()) {
        return;
    }

    /**
     * PERFORM FFT 
     * To translate and synthesize bass, retrieve the major peaks data for
     * the 0 - 100 Hz range and trigger the bass grains with the found
     * frequencies and amplitudes.
     * -- Retrieve MajorPeaks data
    */
    vapi.processInput();
    vapi.analyze();
    Serial.printf("1: Input analyzed\n");

    /**
     * GENERATE BASS OUTPUT
     * To translate and synthesize bass, retrieve the major peaks data for
     * the 0 - 100 Hz range and trigger the bass grains with the found
     * frequencies and amplitudes.
     * -- Retrieve MajorPeaks data
    */
    float **bass_data = bass_peaks.getOutput();
    vapi.mapAmplitudes(bass_data[MP_AMP], NUM_BASS_PEAKS, 250);
    // -- Trigger grains with frequencies and grains
    vapi.triggerGrains(NUM_BASS_PEAKS, bass_data, bass_grains);
    Serial.printf("2: Bass triggered\n");

    /**
     * GENERATE MIDRANGE OUTPUT
     * Frequencies in the 300-800 Hz range are analyzed to pull out melodies
     * from vocals and lead instruments.
     * 
     * The noisiness module measures how noisy the input audio is. A lower
     * noise level improves the ability of MajorPeaks to grab relevant notes.
     * Noisiness is measured as a 0-1 float value, so the threshold of 0.1
     * is used to filter for clear windows to grab melodies in.
     *
     * When an input window is below the noise threshold:
     * 1. Retrieve the MajorPeaks data for 300-800 Hz
     * 2. Transpose detected notes down by octaves until they're below 200 Hz
     * 3. Trigger the first set of mid-range grains with this frequency.
     * 4. Trigger a second set of grains an octave beneath the first. This helps
     * "fill out" the haptic output.
    */
    if (noisiness.getOutput() < 0.1) {

        // Retrieve the MajorPeaks data for the mid range
        float **mid_data = mid_peaks.getOutput();
        vapi.mapAmplitudes(mid_data[MP_AMP], NUM_MID_PEAKS, 550);

        // Transpose detected notes down by octaves until they're below 200 Hz.
        // This places all notes in the range of 100 - 200 Hz
        // This is done to keep notes from going too low, because a lower octave
        // is added beneath these frequencies.
        for(int i=0; i<NUM_MID_PEAKS; i++) {    
            while(mid_data[MP_FREQ][i] > 200) {
                mid_data[MP_FREQ][i] /= 2.0;
            }
        }

        // Trigger the mid-range grains
        vapi.triggerGrains(NUM_MID_PEAKS, mid_data, mid_grains);

        // Transpose notes down another octave
        for(int i=0; i<NUM_MID_PEAKS; i++) {
            mid_data[MP_FREQ][i] /= 2.0;
        }

        // Trigger second set of mid-range grains to "fill out" the output
        vapi.triggerGrains(NUM_MID_PEAKS, mid_data, mid_low_grains);
        Serial.printf("3: Mid-range triggered\n");
    }

    /**
     * GENERATE SNARE OUTPUT
     * A haptic snare response is synthesized when 2 conditions are satisfied:
     * Condition 1: The snare detector detects percussion.
     * -- The snare detector analyzes the 2000 - 4000 Hz range for transients.
     * -- If the snare detector detects a snare, getOutput() returns True.
     * Condition 2: The snare grain is ready to be retriggered.
     * -- Grains run for multiple windows. After completing the RELEASE phase,
     * they enter the READY phase, indicating it's not currently running.
     * -- This condition is needed to prevent the snare from triggering too
     * frequently.
     *
     * To generate a haptic snare response, the following 2 steps occur:
     * 1. Trigger the snare grain with a calculated frequency and amplitude
     * -- the snare grain's amplitude is related to the mean amplitude between 2000-4000 Hz
     * -- the snare grain's frequency is related to the bass data calculated by bass_peaks
     * 2. Retrigger bass grains with a reduced amplitude to make room for snare.
     * - the amplitude of the bass is lowered in proportion to the amplitude of the snare grain
    */
    if(snare_detector.getOutput() && snare_grain.getGrainState() == READY){

        // SNARE AMPLITUDE CALCULATION
        // Mean amplitude in the 2000 - 4000 Hz range is taken as a heuristic
        // for the volume of the percussion that triggered the snare_detector.
        // -- Measure the mean amplitude between 2000-4000 Hz
        float mean = meanAmp.getOutput();
        vapi.mapAmplitudes(&mean, 1, 75);
        // -- Adjust the dynamic range of snare triggerings
        float snare_amp = pow(mean, 3.0);
        float boost = 8.0;
        snare_amp = min(float(snare_amp * boost), float(1.0));

        /**
         * SNARE FREQUENCY CALCULATION
         * -- The snare frequency is taken from the bass analysis. The snare is
         * played on the same haptic driver as the bass, so the snare is set 
         * an octave (or two) up from the bass to reduce phase disturbances
         * and create tactile variation. 
         * -- The snare is transposed up by octaves until it is above 100 Hz to 
         * keep it in the upper haptic range.
        */
        float snare_freq = bass_data[MP_FREQ][0] * 2.0;
        if(snare_freq < 100){ snare_freq *= 2.0; }
        if(snare_freq == 0){ snare_freq = 200; }

        /**
         * TRIGGER GRAINS
         * -- The snare grain is triggered with the frequency and amplitude found
         * above.
        */
        snare_grain.setSustain(snare_freq, snare_amp, 1);
        snare_grain.setRelease(0, 0, 4);

        /**
         * BASS DUCKING
         * -- The snare amplitude is used to duck the volume of the bass, 
         * making the snare more prominant and reducing competition on the 
         * single haptic driver, so the bass grains need to be retriggered.
         * -- NOTE: This is a preliminary implementation for experimentation. It
         * can be commented out without significant effect. It's left in the
         * code so the idea can be expanded on or ruled out.
         * -- For all bass grains, stop the current grain and retrigger with
         * a reduced amplitude.
        */
        for(int i=0; i<NUM_BASS_PEAKS; i++) {
            // Stop the grain (if running)
            // Retrigger the grains with reduced amplitude (minimum 0);
            bass_data[MP_AMP][i] = max(float(0), float(bass_data[MP_AMP][i] - snare_amp));
        }
        vapi.triggerGrains(NUM_BASS_PEAKS, bass_data, bass_grains);

        Serial.printf("4: Snare triggered\n");
    }

    vapi.updateGrains();
    Serial.printf("4: Grains updated\n");
    AudioLab.synthesize();
    AudioLab.printWaves(); // Optionally print synthesized waves every window
}
