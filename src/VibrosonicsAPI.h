#ifndef VIBROSONICS_API_H
#define VIBROSONICS_API_H

// standard library includes
#include <cmath>

// external dependencies
#include <Arduino.h>
#include <AudioLab.h>
#include <AudioPrism.h>
#include <arduinoFFT.h>
#include <cstdint>

// internal
#include "Grain.h"
#include "Wave.h"

constexpr int WINDOW_SIZE_BY_2 = WINDOW_SIZE >> 1;

//! Frequency range of an FFT bin in Hz.
//!
//! The resolution is the frequency range, in Hz, represented by each
//! output bin of the Fast Fourier Transform.
//!
//! Ex: 8192 Samples/Second / 256 Samples/Window = 32 Hz per output bin.
constexpr float FREQ_RES = float(SAMPLE_RATE) / WINDOW_SIZE;

//! Duration of a window, in seconds.
//!
//! The width is the duration of time represented by a
//! single window's worth of samples.
//! Ex: 256 Samples/Window / 8192 Samples/Second = 0.03125 Seconds/Window.
constexpr float FREQ_WIDTH = 1.0 / FREQ_RES;

//! Number of previous windows to be stored and used by modules
const int NUM_WINDOWS = 8;

class VibrosonicsAPI {
    // === PUBLIC DATA & INTERFACE =================================================
public:
    // ---- Setup ------------------------------------------------------------------

    void init();

    // --- FFT Input & Storage -----------------------------------------------------

    // we will need more processAudioInput functionality to handle stereo input

    //! Perform fast fourier transform on the AudioLab input buffer.
    void processAudioInput(float output[]);

    //! Floors data that is below a certain threshold.
    void noiseFloor(float data[], int dataLength, float threshold);

    //! Floors data using the CFAR algorithm.
    void noiseFloorCFAR(float data[], int dataLength, int numRefs, int numGuards, float bias);

    // --- AudioLab Interactions ---------------------------------------------------

    //! Add a wave to a channel with specified frequency and amplitude.
    void assignWave(float freq, float amp, int channel);

    //! Add waves to a channel from the values in the frequency and amplitude
    //! arrays.
    void assignWaves(float* freqs, float* amps, int dataLength, int channel);

    // --- Wave Manipulation -------------------------------------------------------

    //! Returns the mean of some data.
    float getMean(float* data, int dataLength);

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
    // --- ArduinoFFT library ------------------------------------------------------

    //! FFT stores the fourier transform engine.
    ArduinoFFT<float> FFT = ArduinoFFT<float>();

    // Fast Fourier Transform uses complex numbers
    float vReal[WINDOW_SIZE]; //!< Real component of cosine amplitude of each frequency.
    float vImag[WINDOW_SIZE]; //!< Imaginary component of the cosine amplitude of each frequency.

    // --- AudioLab Library --------------------------------------------------------

    //! VibrosonicsAPI adopts AudioLab's input buffer.
    int* audioLabInputBuffer = AudioLab.getInputBuffer();

    GrainList grainList;
};

#endif // VIBROSONICS_API_H
