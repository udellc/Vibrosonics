#ifndef VIBROSONICS_API_H
#define VIBROSONICS_API_H

// standard library includes
#include <cmath>

// external dependencies
#include "Arduino.h"
#include "Modules.h"
#include <AudioLab.h>
#include <AudioPrism.h>
#include <Fast4ier.h>
#include <complex.h>
#include <cstdint>

// internal
#include "Grain.h"
#include "Spectrogram.h"
#include "Wave.h"

class VibrosonicsAPI {
    // === PUBLIC DATA & INTERFACE =================================================
public:
    // --- Constants ---------------------------------------------------------------

    // Some contants are defined to save repeated calculations
    // WINDOW_SIZE and SAMPLE_RATE are defined in the AudioLab library

    //! Half the window size.
    //!
    //! This constant is used often, becuase the output of a fast fourier
    //! transform has half as many values as the input window size.
    static const int WINDOW_SIZE_BY_2 = int(WINDOW_SIZE) >> 1;

    //! Frequency range of an FFT bin in Hz.
    //!
    //! The resolution is the frequency range, in Hz, represented by each
    //! output bin of the Fast Fourier Transform.
    //!
    //! Ex: 8192 Samples/Second / 256 Samples/Window = 32 Hz per output bin.
    const float FREQ_RES = float(SAMPLE_RATE) / WINDOW_SIZE;

    //! Duration of a window, in seconds.
    //!
    //! The width is the duration of time represented by a
    //! single window's worth of samples.
    //! Ex: 256 Samples/Window / 8192 Samples/Second = 0.03125 Seconds/Window.
    const float FREQ_WIDTH = 1.0 / FREQ_RES;

    //! Number of previous windows to be stored and used by modules
    const static int NUM_WINDOWS = 8;

    // ---- Setup ------------------------------------------------------------------

    void init();

    // --- FFT Input & Storage -----------------------------------------------------

    //! Perform fast fourier transform on the AudioLab input buffer.
    void performFFT(int* input);

    //! Pre compute hamming windows for FFT operations
    void computeHammingWindow();

    //! Perform DC Removal to reduce noise in vReal.
    void dcRemoval();

    //! Applies windowing function to vReal data.
    void fftWindowing();

    //! Computes frequency magnitudes in vReal data.
    void complexToMagnitude();

    //! Process the AudioLab input into FFT data and store in the spectrogram
    //! buffer.
    void processInput();

    //! Process the AudioLab input into FFT data and minimize noise before
    //! storing in the spectrogram.
    void processInput(float noiseThreshold);

    //! Process the AudioLab input into FFT data and minimize noise with CFAR
    //! before storing in the spectrogram.
    void processInput(int numRefs, int numGuards, float bias);

    //! Floors data that is below a certain threshold.
    void noiseFloor(float* data, int dataLength, float threshold);

    //! Floors data using the CFAR algorithm.
    void noiseFloorCFAR(float* data, int dataLength, int numRefs, int numGuards, float bias);

    //! Get the current spectrogram window data.
    float* getCurrentWindow() const;

    //! Get the previous spectrogram window data.
    float* getPreviousWindow() const;

    //! Get a spectrogram window at a relative index.
    float* getWindowAt(int relativeIndex) const;

    // --- AudioPrism Management ---------------------------------------------------

    //! Add an AudioPrism module to our loaded modules.
    void addModule(AnalysisModule* module);

    //! Add an AudioPrism module to our loaded modules and bind it to a
    //! frequency range.
    void addModule(AnalysisModule* module, int lowerFreq, int upperFreq);

    //! Runs doAnalysis() for all added AudioPrism modules.
    void analyze();

    // --- AudioLab Interactions ---------------------------------------------------

    //! Add a wave to a channel with specified frequency and amplitude.
    void assignWave(float freq, float amp, int channel);

    //! Add waves to a channel from the values in the frequency and amplitude
    //! arrays.
    void assignWaves(float* freqs, float* amps, int dataLength, int channel);

    // --- Wave Manipulation -------------------------------------------------------

    //! Returns the mean of some float data.
    float getMean(float* data, int dataLength);

    //! Retruns the mean of some complex data.
    float getMean(complex* data, int dataLength);

    //! Maps amplitudes in some data to between 0.0-1.0 range.
    void mapAmplitudes(float* ampData, int dataLength, float dataSumFloor);

    //! linearly maps input frequencies from (0 - (1/2)*SAMPLE_RATE) Hz to
    //! (0 - 250) Hz, the haptic range.
    void mapFrequenciesLinear(float* freqData, int dataLength);

    //! Exponentially maps input frequencies from (0 - (1/2)*SAMPLE_RATE) Hz to
    //! (0 - 250) Hz, the haptic range.
    void mapFrequenciesExponential(float* freqData, int dataLength, float exp);

    // --- Grains -----------------------------------------------------------------

    //! Updates all grains in the globalGrainList
    void updateGrains();

    //! Creates and returns an array of grains on desired chanel with specified wave type.
    Grain* createGrainArray(int numGrains, uint8_t channel, WaveType waveType);

    //! Creates and returns a singular grain with specified wave and channel.
    Grain* createGrain(uint8_t channel, WaveType waveType);

    //! Updates an array of numPeaks grains sustain and release windows.
    void triggerGrains(Grain* grains, int numPeaks, float** peakData);

    // GRAIN SHAPING

    void shapeGrainAttack(Grain* grains, int numGrains, int duration, float freqMod, float ampMod, float curve);

    void shapeGrainSustain(Grain* grains, int numGrains, int duration, float freqMod, float ampMod);

    void shapeGrainRelease(Grain* grains, int numGrains, int duration, float freqMod, float ampMod, float curve);

    // === PRIVATE DATA ============================================================
private:
    //! Static memory allocation for our Spectrogram which stores FFT result
    //! data.
    float spectrogramBuffer[NUM_WINDOWS][WINDOW_SIZE_BY_2];

    //! The spectrogram holds frequency domain data over multiple windows of time.
    Spectrogram<float> spectrogram = Spectrogram((float*)spectrogramBuffer, NUM_WINDOWS, WINDOW_SIZE_BY_2);

    // --- ArduinoFFT library ------------------------------------------------------

    // Fast Fourier Transform uses complex numbers
    float vReal[WINDOW_SIZE]; //!< Real component of cosine amplitude of each frequency.
    float hamming[WINDOW_SIZE]; //!< Pre computed hamming window data
    complex vData[WINDOW_SIZE];

    // --- AudioLab Library --------------------------------------------------------

    //! VibrosonicsAPI adopts AudioLab's input buffer.
    int* AudioLabInputBuffer = AudioLab.getInputBuffer();

    // --- AudioPrism Library ------------------------------------------------------

    // This library integrates AudioPrism by storing an array of analysis
    // modules to keep updated. Users are expected to instance modules and use
    // their results on their own.

    AnalysisModule** modules; //!< Array of references to AudioPrism modules.
    int numModules = 0; //!< Used to track the number of loaded AudioPrism modules.

    GrainList grainList;
};

#endif // VIBROSONICS_API_H
