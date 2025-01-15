#ifndef VIBROSONICS_API_H
#define VIBROSONICS_API_H

// standard library includes
#include <cmath>

// external dependencies
#include <AudioLab.h>
#include <AudioPrism.h>
#include <arduinoFFT.h>
#include "Modules.h"

// internal
#include "CircularBuffer.h"
#include "Grain.h"

class VibrosonicsAPI {
    // === PRIVATE DATA ============================================================
private:
    // --- ArduinoFFT library ------------------------------------------------------

    /** FFT stores the fourier transform engine */
    ArduinoFFT<float> FFT = ArduinoFFT<float>();

    // Fast Fourier Transform uses complex numbers
    float vReal[WINDOW_SIZE]; /** Real component of cosine amplitude of each frequency */
    float vImag[WINDOW_SIZE]; /** Imaginary component of the cosine amplitude of each frequency */

    // --- AudioLab Library --------------------------------------------------------

    /** VibrosonicsAPI adopts AudioLab's input buffer */
    int* AudioLabInputBuffer = AudioLab.getInputBuffer();

    // --- AudioPrism Library ------------------------------------------------------

    // AUDIOPRISM modules can be registered with the Vibrosonics API using the
    // VibrosonicsAPI::addModule() function. This array stores references to all
    // AudioPrism modules registered with an instance of the API. This allows
    // automatic synchronization of module's audio context (sample rate and
    // window size) and allows performing simultaneous analysis on multiple
    // modules with a single call to VibrosonicsAPI::analyze()

    AnalysisModule** modules; /** array of references to AudioPrism modules */
    int numModules = 0; /** integer used to track the array size */

    // === PUBLIC DATA & INTERFACE =================================================
public:
    // --- Constants ---------------------------------------------------------------

    // Some contants are defined to save repeated calculations
    // WINDOW_SIZE and SAMPLE_RATE are defined in the AudioLab library

    /**
     * Half the window size.
     * This constant is used often, becuase the output of a fast fourier
     * transform has half as many values as the input window size.
     */
    static const int windowSizeBy2 = int(WINDOW_SIZE) >> 1;

    /**
     * Frequency range of an FFT bin in Hz.
     * The resolution is the frequency range, in Hz,
     * represented by each output bin of the Fast Fourier Transform.
     * Ex: 8192 Samples/Second / 256 Samples/Window = 32 Hz per output bin
     */
    const float frequencyResolution = float(SAMPLE_RATE) / WINDOW_SIZE;

    /**
     * Duration, in seconds, of a window
     * * The width is the duration of time represented by a
     * single window's worth of samples
     * Ex: 256 Samples/Window / 8192 Samples/Second = 0.03125 Seconds/Window
     */
    const float frequencyWidth = 1.0 / frequencyResolution;

    // ---- Setup ------------------------------------------------------------------

    /** This is optional */
    void init();

    // --- FFT Input & Storage -----------------------------------------------------

    /** Static memory allocation for our circular buffer which stores FFT result data. */
    float staticBuffer[2][windowSizeBy2];

    /**
     * The circular buffer is used to efficiently store and retrieve multiple
     * audio spectrums.
     * Pushing new items on the circular buffer overwrites the
     * oldest items, rather than shifting memory around.
     * -- The first argument of the constructor is the number of audio spectrums
     * to store. This is set to two because none of our analysis requires a
     * longer history. This value can be increased arbitrarily to store longer
     * histories.
     * -- The second argument of the constructor is the number of bins in each
     * audio spectrum. This will be the result of an FFT operation, so it must
     * be set to half the window size used for the FFT.
     * -- Note: AudioPrism modules take regular 2D float arrays as input. Call
     * CircularBuffer::unwind to get a flat 2D version of the buffer's content.
     */
    CircularBuffer<float> circularBuffer = CircularBuffer((float*)staticBuffer, 2, windowSizeBy2);

    /**
     * storeFFTData writes the result of the most recent FFT computation into
     * the circular buffer.
     */
    void storeFFTData();

    /**
     * performFFT feeds its input array to the ArduinoFFT fourier transform
     * engine and stores the results in private data members vReal and vImag.
     */
    void performFFT(int* input);

    /**
     * processInput calls performFFT and storeFFTData to compute and store
     * the FFT of AudioLab's input buffer.
     */
    void processInput();

    // --- AudioPrism Management ---------------------------------------------------

    /** add a module to list of submodules */
    void addModule(AnalysisModule* module);

    /** runs doAnalysis() for all added submodules */
    void analyze();

    // --- AudioLab Interactions ---------------------------------------------------

    /**
     * assignWave creates and adds a wave to a channel for output. The wave is
     * synthesized from the provided frequency and amplitude.
     */
    void assignWave(float freq, float amp, int channel);

    /**
     * assignWaves creates and adds multiple waves to a channel for output. The
     * waves are synthesized from the frequencies and amplitudes provided as
     * arguments. Both frequency and amplitude arrays must be equal lengthed,
     * and their length must be passed as dataLength.
     */
    void assignWaves(float* freqs, float* amps, int dataLength, int channel);

    // --- Wave Manipulation -------------------------------------------------------

    /** returns the mean of some data */
    float getMean(float* data, int dataLength);

    /** floors data that is below a certain threshold */
    void noiseFloor(float* data, float threshold);

    /** maps amplitudes in some data to between 0-127 range */
    void mapAmplitudes(float* ampData, int dataLength, float maxDataSum);

    /**
     * linearly maps input frequencies from (0 - (1/2)*SAMPLE_RATE) Hz to
     * (0 - 250) Hz, the haptic range.
     * mapFrequenciesLinear() and mapFrequencyExponential() map their input
     * frequencies to the haptic range (0-250).
     * -- These functions help reduce 'R2-D2' noises caused by outputting high
     * frequencies.
     * -- We've found that maintaining certain harmonic relationships between
     * frequencies for output on a single driver can greatly improve tactile
     * feel, so we recommend scaling down by octaves in these scenarios.
     */
    void mapFrequenciesLinear(float* freqData, int dataLength);

    /**
     * exponentially maps input frequencies from (0 - (1/2)*SAMPLE_RATE) Hz to
     * (0 - 250) Hz, the haptic range.
     * mapFrequenciesLinear() and mapFrequencyExponential() map their input
     * frequencies to the haptic range (0-250).
     * -- These functions help reduce 'R2-D2' noises caused by outputting high
     * frequencies.
     * -- We've found that maintaining certain harmonic relationships between
     * frequencies for output on a single driver can greatly improve tactile
     * feel, so we recommend scaling down by octaves in these scenarios.
     */
    void mapFrequenciesExponential(float* freqData, int dataLength, float exp);
};

#endif // VIBROSONICS_API_H
