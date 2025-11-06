#include "VibrosonicsAPI.h"
#include "Config.h"
#include "Grain.h"
#include "Spectrogram.h"
#include <cmath>

/**
 * Initializes all necessary api variables and dependencies.
 */
void VibrosonicsAPI::init()
{
    AudioLab.init();
    this->computeHammingWindow();
}

/**
 * Feeds its input array to the Fast4ier fourier transform engine
 * and stores the results in private data members vData and vReal.
 *
 * @param output Array of signals to store result of FFT operations in.
 */
void VibrosonicsAPI::processAudioInput(float output[])
{
    // Copy samples from rolling buffer to FFT input array
    for (int i = 0; i < WINDOW_SIZE_OVERLAP; i++) {
      vData[i] = rollingInputBuffer[i];
    }

    // Use Fast4ier combined with Vibrosonics FFT functions
    dcRemoval();
    fftWindowing();
    Fast4::FFT(vData, WINDOW_SIZE_OVERLAP);
    complexToMagnitude();

    // Copy complex data to float arrays
    for (int i = 0; i < WINDOW_SIZE_OVERLAP; i++) {
        vReal[i]  = vData[i].re();
        output[i] = vReal[i];
    }

    // Move rolling buffer forward to make room for next set of samples
    for (int i = WINDOW_SIZE; i < WINDOW_SIZE_OVERLAP; i++) {
      rollingInputBuffer[i - WINDOW_SIZE] = rollingInputBuffer[i];
    }
}

/**
 * Removes the mean of the data from each bin to reduce noise.
 */
void VibrosonicsAPI::dcRemoval()
{
    float mean = getMean(vData, WINDOW_SIZE_OVERLAP);
    for (int i = 0; i < WINDOW_SIZE_OVERLAP; i++) {
        vData[i] -= mean;
    }
}

void VibrosonicsAPI::computeHammingWindow()
{
    float step = 2 * PI / (WINDOW_SIZE_OVERLAP - 1);
    for (int i = 0; i < WINDOW_SIZE_OVERLAP; i++) {
        hamming[i] = 0.54 - 0.46 * cos(step * i);
    }
}

/**
 * Applies a precomputed hamming windowing factor to the data.
 * This is done to reduce spectral leakage between bins.
 */
void VibrosonicsAPI::fftWindowing()
{
    for (int i = 0; i < WINDOW_SIZE_OVERLAP; i++) {
        // Use precomputed hamming window for better efficiency. Thanks Nick!
        vData[i] *= hamming[i];
    }
}

/**
 * Converts raw FFT output to a readable frequency spectrogram.
 */
void VibrosonicsAPI::complexToMagnitude()
{
    for (int i = 0; i < WINDOW_SIZE_OVERLAP; i++) {
        vData[i] = sqrt(pow(vData[i].re(), 2) + pow(vData[i].im(), 2));
    }
}

/**
 * Finds and returns the mean value of float data.
 *
 * @param data Array of amplitudes.
 * @param dataLength Length of amplitude array.
 */
float VibrosonicsAPI::getMean(float* data, int dataLength)
{
    float sum = 0.0;
    for (int i = 0; i < dataLength; i++) {
        sum += data[i];
    }
    return sum > 0.0 ? sum / dataLength : sum;
}

/**
 * Finds and returns the mean value of complex data.
 *
 * @param data Array of amplitudes.
 * @param dataLength Length of amplitude array.
 */
float VibrosonicsAPI::getMean(complex* data, int dataLength)
{
    float sum = 0.0;
    for (int i = 0; i < dataLength; i++) {
        sum += data[i].re();
    }
    return sum > 0.0 ? sum / dataLength : sum;
}

/**
 * Sets the amplitude of a bin to 0 if it is less than threshold.
 *
 * @param windowData The frequency domain data to filter.
 * @param threshold The threshold value to floor the data at.
 */
void VibrosonicsAPI::noiseFloor(float* ampData, float threshold)
{
    for (int i = 0; i < WINDOW_SIZE_BY_2; i++) {
        if (ampData[i] < threshold) {
            ampData[i] = 0.0;
        }
    }
}

/**
 * Uses a sliding window to compute the average value of a number of reference
 * cells for each cell under test (CUT). If the CUT's value is less than the
 * average * a bias, then the cell is floored.
 *
 * Read more on the CFAR here:
 * https://en.wikipedia.org/wiki/Constant_false_alarm_rate.
 *
 * @param windowData The frequency domain data to filter.
 * @param numRefs The number of reference cells for CFAR.
 * @param numGuards The number of guard cells for CFAR.
 * @param bias The bias factor to use for CFAR.
 */
void VibrosonicsAPI::noiseFloorCFAR(float* windowData, int numRefs, int numGuards, float bias)
{
    int   i, j, numCells;
    int   left_start, left_end, right_start, right_end;
    float noiseLevel;

    // copy data for output
    float dataCopy[WINDOW_SIZE_BY_2];
    memcpy(dataCopy, windowData, WINDOW_SIZE_BY_2 * sizeof(float));

    for (i = 0; i < WINDOW_SIZE_BY_2; i++) {
        // calculate bounds for cell under test (CUT)
        left_start  = max(0, i - numGuards - numRefs);
        left_end    = max(0, i - numGuards);
        right_start = min(WINDOW_SIZE_BY_2, i + numGuards);
        right_end   = min(WINDOW_SIZE_BY_2, i + numGuards + numRefs);

        numCells   = 0;
        noiseLevel = 0;

        // compute sum of cells within bounds
        for (j = left_start; j < left_end; j++) {
            noiseLevel += dataCopy[j];
            numCells++;
        }
        for (j = right_start; j < right_end; j++) {
            noiseLevel += dataCopy[j];
            numCells++;
        }

        // compute average (noise level) of cells within bounds
        noiseLevel /= numCells > 0 ? numCells : 1;

        // copy original data if above noiseLevel, otherwise floor the CUT
        if (dataCopy[i] > noiseLevel * bias) {
            windowData[i] = dataCopy[i];
        } else {
            windowData[i] = 0;
        }
    }
}

/**
 * Maps amplitudes to the range [0, 1] by normalizing them by the sum of
 * the amplitudes. This sum is smoothed by the previous data to ensure a
 * consistent amplitude output and contrast.
 *
 * @param ampData The amplitude array to map.
 * @param dataLength The length of the amplitude array.
 * @param minAmpSum The minimum value to normalize the amplitudes by (if
 * their sum is not greater than it).
 * @param smoothFactor The factor to smooth the amplitudes by.
 */
void VibrosonicsAPI::mapAmplitudes(float* ampData, int dataLength,
    float minAmpSum, float smoothFactor)
{
    if (smoothFactor < 0.0 || smoothFactor > 1.0) {
        Serial.printf("Error: smoothFactor must be between 0 and 1.\n");
        return;
    }

    static float runningSum = minAmpSum;

    // sum amp data
    float dataSum = 0.0;
    for (int i = 0; i < dataLength; i++) {
        dataSum += ampData[i];
    }
    if (dataSum == 0.0) {
        return; // return early if sum of amplitudes is 0
    }

    // smooth the sum of amplitudes with previous data unless it is greater
    if (dataSum < runningSum) {
        runningSum = runningSum * (1 - smoothFactor) + dataSum * smoothFactor;
        // dampen noise by clamping to minAmpSum
        if (runningSum < minAmpSum) {
            runningSum = minAmpSum;
        }
    } else {
        runningSum = dataSum;
    }

    // convert amplitudes
    for (int i = 0; i < dataLength; i++) {
        ampData[i] = (ampData[i] / runningSum);
    }
}

/**
 * Creates and adds a wave to a channel for output. The wave is
 * synthesized from the provided frequency and amplitude.
 *
 * @param freq Frequency of the synthesized wave.
 * @param amp Amplitude of the synthesized wave.
 * @param channel The output channel to add the wave to.
 */
void VibrosonicsAPI::assignWave(float freq, float amp, int channel)
{
    Wave wave = AudioLab.dynamicWave(channel, freq, amp);
}

/**
 * Creates and adds multiple waves to a channel for output. The
 * waves are synthesized from the frequencies and amplitudes provided as
 * arguments. Both frequency and amplitude arrays must be equal lengthed,
 * and their length must be passed as dataLength.
 *
 * @param freqs Frequencies of the synthesized waves.
 * @param amps Amplitudes of the synthesized waves.
 * @param dataLength The length of the frequency and amplitude arrays.
 * @param channel The output channel to add the waves to.
 */
void VibrosonicsAPI::assignWaves(float* freqData, float* ampData, int dataLength, int channel)
{
    for (int i = 0; i < dataLength; i++) {
        if (ampData[i] == 0.0 || freqData[i] == 0)
            continue;                                                              // skip storing if ampData is 0, or freqData is 0
        Wave wave = AudioLab.dynamicWave(channel, round(freqData[i]), ampData[i]); // create wave
    }
}

/**
 * Checks if the a new audio window has been recorded by seeing if our input buffer is full.
 */
bool VibrosonicsAPI::isAudioLabReady()
{
    return AudioLab.ready<uint16_t>(rollingInputBuffer + WINDOW_SIZE_OVERLAP - WINDOW_SIZE, WINDOW_SIZE_OVERLAP);
}

float VibrosonicsAPI::mapFrequencyByOctaves(float inFreq, float maxFreq)
{
    int   shift = 0;
    float freq  = maxFreq;
    while (freq > 230) {
        freq /= 2;
        shift++;
    }

    freq = inFreq / (1 << shift);

    return freq;
}

// https://newt.phys.unsw.edu.au/jw/notes.html
float VibrosonicsAPI::mapFrequencyMIDI(float inFreq, float minFreq, float maxFreq)
{
    float midi_min = 69 + 12 * log2(minFreq / 440.);
    float midi_max = 69 + 12 * log2(maxFreq / 440.);
    float midi_in  = 69 + 12 * log2(inFreq / 440.);

    if (midi_in < midi_min) {
        midi_in = midi_min;
    } else if (midi_in > midi_max) {
        midi_in = midi_max;
    }

    float ratio = (midi_in - midi_min) / (midi_max - midi_min);

    // use the ratio to map between (80-230 Hz)
    return 80 + ratio * (180 - 80);
}

/**
 * Creates an array of grains with specified length, channel, and wave type and then
 * pushes the newly created grain array to the global grain list.
 *
 * @param numGrains The size of the output Grains array
 * @param channel The physical speaker channel, on current hardware valid inputs are 0-2
 * @param waveType The type of wave Audiolab will generate utilizing the grains.
 */
Grain* VibrosonicsAPI::createGrainArray(int numGrains, uint8_t channel, WaveType waveType)
{
    Grain* newGrains = new Grain[numGrains];

    for (int i = 0; i < numGrains; i++) {
        newGrains[i].setChannel(channel);
        newGrains[i].setWaveType(waveType);
        grainList.pushGrain(&newGrains[i]);
    }

    return newGrains;
}

/**
 * Creates a grain that runs once and is then deleted.
 *
 * @param channel The physical speaker channel, on current hardware valid inputs are 0-2
 * @param waveType The type of wave Audiolab will generate utilizing the grains.
 * @param FreqEnv The frequency data used to shape the grain.
 * @param AmpEnv The amplitude data used to shape the grain.
 * @param durEnv The duration lengths and curve to shape the grain.
 */
Grain* VibrosonicsAPI::createDynamicGrain(uint8_t channel, WaveType waveType, FreqEnv freqEnv, AmpEnv ampEnv, DurEnv durEnv)
{
    Grain* newGrain     = new Grain(channel, waveType);
    newGrain->isDynamic = true;
    grainList.pushGrain(newGrain);
    newGrain->setFreqEnv(freqEnv);
    newGrain->setAmpEnv(ampEnv);
    newGrain->setDurEnv(durEnv);
    newGrain->transitionTo(ATTACK);
    return newGrain;
}

/**
 * Calls update for every grain in the grain list
 * Deletes dynamic grains as needed.
 */
void VibrosonicsAPI::updateGrains()
{
    grainList.updateAndReap();
}

/**
 * Updates an array of grains attack, sustain, and release windows if
 * the data's amplitude is greater than the amplitude stored in the grain.
 *
 * @param grains An array of grains to be triggered.
 * @param numGrains The size of the Grain array.
 * @param freqEnv The struct containing the frequency envelope data
 * @param ampEnv The struct containing the amplitude envelope data
 * @param durEnv The duration lengths and curve to shape the grain.
 */
void VibrosonicsAPI::triggerGrains(Grain* grains, int numGrains, FreqEnv freqEnv, AmpEnv ampEnv, DurEnv durEnv)
{
    for (int i = 0; i < numGrains; i++) {
        if (grains[i].getGrainState() == READY) {
            grains[i].setFreqEnv(freqEnv);
            grains[i].setAmpEnv(ampEnv);
            grains[i].setDurEnv(durEnv);
            grains[i].transitionTo(ATTACK);
        }
    }
}

/**
 * Creates a frequency envelope struct to shape the grain's frequency parameters with
 *
 * @param attackFreq The frequency the grain will target in its attack state
 * @param decayFreq The frequency the grain will target in its decay state
 * @param sustainFreq The frequency the grain will target in its sustain state
 * @param releaseFreq The frequency the grain will target in its release state
 */
FreqEnv VibrosonicsAPI::createFreqEnv(float attackFreq, float decayFreq, float sustainFreq, float releaseFreq)
{
    FreqEnv newFreqEnv = { attackFreq, decayFreq, sustainFreq, releaseFreq };
    return newFreqEnv;
}

/**
 * Creates a amplitude envelope struct to shape the grain's amplitude parameters with
 * Also sets state durations and curve shape.
 *
 * @param attackAmp The amplitude the grain will target in its attack state
 * @param decayAmp The amplitude the grain will target in its decay state
 * @param sustainAmp The amplitude the grain will target in its sustain state
 * @param releaseAmp The amplitude the grain will target in its release state
 */
AmpEnv VibrosonicsAPI::createAmpEnv(float attackAmp, float decayAmp, float sustainAmp, float releaseAmp)
{
    AmpEnv newAmpEnv = { attackAmp, decayAmp, sustainAmp, releaseAmp };
    return newAmpEnv;
}

/**
 * Creates a duration envelope struct to shape the grain's amplitude parameters with
 * Also sets state durations and curve shape.
 *
 * @param attackDuration The number of windows the attack phase will last for
 * @param decayDuration The number of windows the decay phase will last for
 * @param sustainDuration The number of windows the sustain phase will last for
 * @param releaseDuration The number of windows the release phase will last for
 * @param curve The shape of the grain's progression through targeted amplitudes and frequencies
 */
DurEnv VibrosonicsAPI::createDurEnv(int attackDuration, int decayDuration, int sustainDuration, int releaseDuration, float curve)
{
    DurEnv newDurEnv = { attackDuration, decayDuration, sustainDuration, releaseDuration, curve };
    return newDurEnv;
}

/**
 * Sets the frequency envelope for an array of grains
 *
 * @param grains An array of grains to have their frequency envelope set.
 * @param numGrains The size of the Grain array.
 * @param freqEnv The struct containing the new frequency envelope data
 */
void VibrosonicsAPI::setGrainFreqEnv(Grain* grains, int numGrains, FreqEnv freqEnv)
{
    for (int i = 0; i < numGrains; i++) {
        grains[i].setFreqEnv(freqEnv);
    }
}

/**
 * Sets the amplitude envelope for an array of grains
 *
 * @param grains An array of grains to have their amplitude envelope set.
 * @param numGrains The size of the Grain array.
 * @param ampEnv The struct containing the new amplitude envelope data
 */
void VibrosonicsAPI::setGrainAmpEnv(Grain* grains, int numGrains, AmpEnv ampEnv)
{
    for (int i = 0; i < numGrains; i++) {
        grains[i].setAmpEnv(ampEnv);
    }
}

/**
 * Sets the amplitude envelope for an array of grains
 *
 * @param grains An array of grains to have their amplitude envelope set.
 * @param numGrains The size of the Grain array.
 * @param durEnv The struct containing the new duration envelope data
 */
void VibrosonicsAPI::setGrainDurEnv(Grain* grains, int numGrains, DurEnv durEnv)
{
    for (int i = 0; i < numGrains; i++) {
        grains[i].setDurEnv(durEnv);
    }
}
