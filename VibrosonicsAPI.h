#ifndef VIBROSONICS_API_H
#define VIBROSONICS_API_H

// standard library includes
#include <cmath>

// external dependencies
#include "Modules.h"
#include <AudioLab.h>
#include <AudioPrism.h>
#include <arduinoFFT.h>
#include <cstdint>

// internal
#include "Spectrogram.h"
#include "Grain.h"
#include "Wave.h"

class VibrosonicsAPI {
    // === PRIVATE DATA ============================================================
private:
    // --- ArduinoFFT library ------------------------------------------------------

    //! FFT stores the fourier transform engine.
    ArduinoFFT<float> FFT = ArduinoFFT<float>();

    // Fast Fourier Transform uses complex numbers
    float vReal[WINDOW_SIZE]; //!< Real component of cosine amplitude of each frequency.
    float vImag[WINDOW_SIZE]; //!< Imaginary component of the cosine amplitude of each frequency.

    // --- AudioLab Library --------------------------------------------------------

    //! VibrosonicsAPI adopts AudioLab's input buffer.
    int* AudioLabInputBuffer = AudioLab.getInputBuffer();

    // --- AudioPrism Library ------------------------------------------------------

    // AudioPrism modules can be registered with the Vibrosonics API using the
    // VibrosonicsAPI::addModule() function. This array stores references to all
    // AudioPrism modules registered with an instance of the API. This allows
    // automatic synchronization of module's audio context (sample rate and
    // window size) and allows performing simultaneous analysis on multiple
    // modules with a single call to VibrosonicsAPI::analyze()

    AnalysisModule** modules; //!< Array of references to AudioPrism modules.
    int numModules = 0; //!< Used to track the number of loaded AudioPrism modules.

    GrainList grainList;

    // === PUBLIC DATA & INTERFACE =================================================
public:
    // --- Constants ---------------------------------------------------------------

    // Some contants are defined to save repeated calculations
    // WINDOW_SIZE and SAMPLE_RATE are defined in the AudioLab library

    //! Half the window size.
    //!
    //! This constant is used often, becuase the output of a fast fourier
    //! transform has half as many values as the input window size.
    static const int windowSizeBy2 = int(WINDOW_SIZE) >> 1;

    //! Frequency range of an FFT bin in Hz.
    //!
    //! The resolution is the frequency range, in Hz,
    //! represented by each output bin of the Fast Fourier Transform.
    //! Ex: 8192 Samples/Second / 256 Samples/Window = 32 Hz per output bin.
    const float frequencyResolution = float(SAMPLE_RATE) / WINDOW_SIZE;

    //! Duration of a window, in seconds.
    //!
    //! The width is the duration of time represented by a
    //! single window's worth of samples.
    //! Ex: 256 Samples/Window / 8192 Samples/Second = 0.03125 Seconds/Window.
    const float frequencyWidth = 1.0 / frequencyResolution;

    // ---- Setup ------------------------------------------------------------------

    void init();

    // --- FFT Input & Storage -----------------------------------------------------

    //! Static memory allocation for our circular buffer which stores FFT result data.
    float spectrogramBuffer[2][windowSizeBy2];

    //! The circular buffer is used to efficiently store and retrieve multiple
    //! audio spectrums.
    //!
    //! Pushing new items on the circular buffer overwrites the
    //! oldest items, rather than shifting memory around.
    //!
    //! The first argument of the constructor is the number of audio spectrums
    //! to store. This is set to two because none of our analysis requires a
    //! longer history. This value can be increased arbitrarily to store longer
    //! histories.
    //!
    //! The second argument of the constructor is the number of bins in each
    //! audio spectrum. This will be the result of an FFT operation, so it must
    //! be set to half the window size used for the FFT.
    //!
    //! Note: AudioPrism modules take regular 2D float arrays as input. Call
    //! CircularBuffer::unwind to get a flat 2D version of the buffer's content.
    Spectrogram<float> spectrogram = Spectrogram((float*)spectrogramBuffer, 2, windowSizeBy2);

    //! Stores the most recent FFT result in circularBuffer.
    void storeFFTData();

    //! Perform fast fourier transform on the AudioLab input buffer.
    void performFFT(int* input);

    //! Process the AudioLab input into FFT data, stored in our circular
    //! buffer.
    void processInput();

    // --- AudioPrism Management ---------------------------------------------------

    //! Add an AudioPrism module to our loaded modules.
    void addModule(AnalysisModule* module);

    //! Runs doAnalysis() for all added AudioPrism modules.
    void analyze();

    // --- AudioLab Interactions ---------------------------------------------------

    //! Add a wave to a channel with specified frequency and amplitude.
    void assignWave(float freq, float amp, int channel);

    //! Add waves to a channel from the values in the frequency and amplitude
    //! arrays.
    void assignWaves(float* freqs, float* amps, int dataLength, int channel);

    // --- Wave Manipulation -------------------------------------------------------

    //! Returns the mean of some data.
    float getMean(float* data, int dataLength);

    //! Floors data that is below a certain threshold.
    void noiseFloor(float* data, float threshold);

    //! Maps amplitudes in some data to between 0-127 range.
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

    //! Sets grains attack parameters
    void setGrainAttack(Grain* grains, int numGrains, float frequency, float amplitude, int duration);

    //! Sets grains attack parameters
    void setGrainSustain(Grain* grains, int numGrains, float frequency, float amplitude, int duration);

    //! Sets grains attack parameters
    void setGrainRelease(Grain* grains, int numGrains, float frequency, float amplitude, int duration);
};

#endif // VIBROSONICS_API_H
