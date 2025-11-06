#ifndef VIBROSONICS_API_H
#define VIBROSONICS_API_H

// standard library includes
#include <cmath>

// external dependencies
#include <Arduino.h>
#include <AudioLab.h>
#include <AudioPrism.h>
#include <Fast4ier.h>

// standard libraries
#include <complex>
#include <cstdint>

// internal
#include "Grain.h"
#include "Wave.h"

constexpr uint16_t WINDOW_SIZE_OVERLAP = WINDOW_SIZE << WINDOW_OVERLAP;
constexpr int WINDOW_SIZE_BY_2 = WINDOW_SIZE_OVERLAP >> 1;

//! Frequency range of an FFT bin in Hz.
//!
//! The resolution is the frequency range, in Hz, represented by each
//! output bin of the Fast Fourier Transform.
//!
//! Ex: 8192 Samples/Second / 256 Samples/Window = 32 Hz per output bin.
constexpr float FREQ_RES = (float)SAMPLE_RATE / (float)WINDOW_SIZE_OVERLAP;

//! Duration of a window, in seconds.
//!
//! The width is the duration of time represented by a
//! single window's worth of samples.
//! Ex: 256 Samples/Window / 8192 Samples/Second = 0.03125 Seconds/Window.
constexpr float FREQ_WIDTH = 1.0 / FREQ_RES;

class VibrosonicsAPI {
public:
    // ---- Setup ------------------------------------------------------------------

    void init();

    // --- FFT Input & Storage -----------------------------------------------------

    // we will need more processAudioInput functionality to handle stereo input

    //! Perform fast fourier transform on the AudioLab input buffer.
    void processAudioInput(float* output);

    //! Pre compute hamming windows for FFT operations
    void computeHammingWindow();

    //! Perform DC Removal to reduce noise in vReal.
    void dcRemoval();

    //! Applies windowing function to vReal data.
    void fftWindowing();

    //! Computes frequency magnitudes in vReal data.
    void complexToMagnitude();

    //! Returns the mean of some float data.
    float getMean(float* data, int dataLength);

    //! Retruns the mean of some complex data.
    float getMean(complex* data, int dataLength);

    //! Floors data that is below a certain threshold.
    void noiseFloor(float* data, float threshold);

    //! Floors data using the CFAR algorithm.
    void noiseFloorCFAR(float* data, int numRefs, int numGuards, float bias);

    // --- AudioLab Interactions ---------------------------------------------------

    //! Add a wave to a channel with specified frequency and amplitude.
    void assignWave(float freq, float amp, int channel);

    //! Add waves to a channel from the values in the frequency and amplitude
    //! arrays.
    void assignWaves(float* freqs, float* amps, int dataLength, int channel);

    //! Check if a new audio window has been recorded
    bool isAudioLabReady();

    // --- Wave Manipulation -------------------------------------------------------

    //! Maps amplitudes in some data to between 0.0-1.0 range.
    void mapAmplitudes(float* ampData, int dataLength, float minAmpSum = 10000,
        float smoothFactor = 0.05);

    //! Maps a frequency to the haptic range (0-230Hz) by transposing it down
    //! by octaves.
    float mapFrequencyByOctaves(float inFreq, float maxFreq);

    //! Maps a frequency to the haptic range by quantizing it using MIDI
    //! values.
    float mapFrequencyMIDI(float inFreq, float minFreq, float maxFreq);

    // --- Grains -----------------------------------------------------------------

    //! Updates all grains in the globalGrainList
    void updateGrains();

    //! Creates and returns a static array of grains on desired chanel with specified
    //! wave type.
    Grain* createGrainArray(int numGrains, uint8_t channel, WaveType waveType);

    //! Creates and returns a single dynamic grain with the specified channel and wave type.
    //! Also takes frequency and amplitude envelopes for immediate triggering.
    Grain* createDynamicGrain(uint8_t channel, WaveType waveType, FreqEnv freqEnv, AmpEnv ampEnv, DurEnv durEnv);

    //! Updates an array of numPeaks grains sustain and release windows.
    void triggerGrains(Grain* grains, int numGrains, FreqEnv freqEnv, AmpEnv ampEnv, DurEnv durEnv);

    //! Creates a frequency envelope for a grain.
    FreqEnv createFreqEnv(float attackFreq, float decayFreq, float sustainFreq, float releaseFreq);

    //! Creates an amplitude envelope for a grain.
    AmpEnv createAmpEnv(float attackAmp, float decayAmp, float sustainAmp, float releaseAmp);

    //! Creates a duration envelope for a grain.
    DurEnv createDurEnv(int attackDuration, int decayDuration, int sustainDuration, int releaseDuration, float curve);

    //! Sets the frequency envelope parameters for an array of grains.
    void setGrainFreqEnv(Grain* grains, int numGrains, FreqEnv freqEnv);

    //! Sets the amplitude envelope parameters for an array of grains.
    void setGrainAmpEnv(Grain* grains, int numGrains, AmpEnv ampEnv);

    //! Sets the duration envelope paramaters for an array of grains.
    void setGrainDurEnv(Grain* grains, int numGrains, DurEnv durEnv);

private:
    // Fast Fourier Transform uses complex numbers
    float   vReal[WINDOW_SIZE_OVERLAP];   //!< Real component of cosine amplitude of each frequency.
    float   hamming[WINDOW_SIZE_OVERLAP]; //!< Pre computed hamming window data
    complex vData[WINDOW_SIZE_OVERLAP];
    uint16_t rollingInputBuffer[WINDOW_SIZE_OVERLAP]; 
    
    // --- AudioLab Library --------------------------------------------------------

    GrainList grainList;
};

#endif // VIBROSONICS_API_H
