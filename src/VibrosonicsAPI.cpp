#include "VibrosonicsAPI.h"
#include "Config.h"
#include "Spectrogram.h"

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
    // copy samples from input to vData and set imaginary component to 0
    for (int i = 0; i < WINDOW_SIZE; i++) {
        vData[i] = audioLabInputBuffer[i];
    }
    // Use Fast4ier combined with Vibrosonics FFT functions
    this->dcRemoval();
    this->fftWindowing();
    Fast4::FFT(vData, WINDOW_SIZE);
    this->complexToMagnitude();

    // Copy complex data to float arrays
    for (int i = 0; i < WINDOW_SIZE; i++) {
        vReal[i] = vData[i].re();
        output[i] = vReal[i];
    }
}

/**
 * Removes the mean of the data from each bin to reduce noise.
 */
void VibrosonicsAPI::dcRemoval()
{
    float mean = this->getMean(vData, WINDOW_SIZE);
    for (int i = 0; i < WINDOW_SIZE; i++) {
        vData[i] -= mean;
    }
}

void VibrosonicsAPI::computeHammingWindow() {
    float step = 2 * PI / (WINDOW_SIZE - 1);
    for (int i = 0; i < WINDOW_SIZE; i++) {
        hamming[i] = 0.54 - 0.46 * cos(step * i);
    }
}

/**
 * Applies a precomputed hamming windowing factor to the data.
 * This is done to reduce spectral leakage between bins.
 */
void VibrosonicsAPI::fftWindowing()
{
    for (int i = 0; i < WINDOW_SIZE; i++) {
        // Use precomputed hamming window for better efficiency. Thanks Nick!
        vData[i] *= hamming[i];
    }
}

/**
 * Converts raw FFT output to a readable frequency spectrogram.
 */
void VibrosonicsAPI::complexToMagnitude()
{
    for (int i = 0; i < WINDOW_SIZE; i++) {
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
 * @param data The array of data to floor.
 * @param threshold The threshold value to floor the data at.
 */
void VibrosonicsAPI::noiseFloor(float* ampData, int dataLength, float threshold)
{
    for (int i = 0; i < dataLength; i++) {
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
 * @param numRefs The number of reference cells for CFAR.
 * @param numGuards The number of guard cells for CFAR.
 * @param bias The bias factor to use for CFAR.
 */

void VibrosonicsAPI::noiseFloorCFAR(float* data, int dataLength, int numRefs, int numGuards, float bias)
{
    int i, j, numCells;
    int left_start, left_end, right_start, right_end;
    float noiseLevel;

    // copy data for output
    float dataCopy[dataLength];
    memcpy(dataCopy, data, dataLength * sizeof(float));

    for (i = 0; i < dataLength; i++) {
        // calculate bounds for cell under test (CUT)
        left_start = max(0, i - numGuards - numRefs);
        left_end = max(0, i - numGuards);
        right_start = min(dataLength, i + numGuards);
        right_end = min(dataLength, i + numGuards + numRefs);

        numCells = 0;
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
            data[i] = dataCopy[i];
        } else {
            data[i] = 0;
        }
    }
}

/**
 * Maps amplitudes in the input array to between 0-127 range.
 *
 * @param ampData The amplitude array to map.
 * @param dataLength The length of the amplitude array.
 * @param minDataSum The minimum value to normalize the amplitudes by (if
 * their sum is not greater than it).
 */
void VibrosonicsAPI::mapAmplitudes(float* ampData, int dataLength, float minDataSum = 0)
{
    // sum amp data
    float dataSum = 0.0;
    for (int i = 0; i < dataLength; i++) {
        dataSum += ampData[i];
    }
    if (dataSum == 0.0) {
        return; // return early if sum of amplitudes is 0
    }
    if (minDataSum > 0 && dataSum < minDataSum) {
        dataSum = minDataSum;
    }

    // minDataSum is a special parameter that will need to be adjusted to
    // find the max dynamic contrast for a given set of ampllitudes. In
    // general, a lower minDataSum will proivde less dynamic contrast (i.e.
    // the range of amplitudes will be compressed), whereas a higher
    // minDataSum may allow more room for contrast. If the sum of amplitudes
    // (dataSum) is larger than minDataSum then the dataSum will be used to
    // ensure values are no larger than 1.

    // convert amplitudes
    for (int i = 0; i < dataLength; i++) {
        ampData[i] = (ampData[i] / dataSum);
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
            continue; // skip storing if ampData is 0, or freqData is 0
        Wave wave = AudioLab.dynamicWave(channel, round(freqData[i]), ampData[i]); // create wave
    }
}

/**
 * mapFrequenciesLinear() and mapFrequenciesExponential() map their input
 * frequencies to the haptic range (0-250Hz).
 *
 * mapFrequenciesLinear maps the frequencies to the lower range by normalizing
 * them and then scaling by 250 Hz.
 *
 * These functions help reduce 'R2-D2' noises caused by outputting high
 * frequencies.
 *
 * We've found that maintaining certain harmonic relationships between
 * frequencies for output on a single driver can greatly improve tactile
 * feel, so we recommend scaling down by octaves in these scenarios.
 *
 * @param freqData The frequency array to map.
 * @param dataLength The length of the frequency array.
 */
void VibrosonicsAPI::mapFrequenciesLinear(float* freqData, int dataLength)
{
    float freqRatio;
    // loop through frequency data
    for (int i = 0; i < dataLength; i++) {
        freqRatio = freqData[i] / (SAMPLE_RATE >> 1); // find where each freq lands in the spectrum
        freqData[i] = round(freqRatio * 250) + 20; // convert into to haptic range linearly
    }
}

/**
 * mapFrequenciesLinear() and mapFrequenciesExponential() map their input
 * frequencies to the haptic range (0-250Hz).
 *
 * mapFrequencyExponential uses an exponent to apply a curve to the mapping
 * that mapFrequenciesLinear does.
 *
 * These functions help reduce 'R2-D2' noises caused by outputting high
 * frequencies.
 *
 * We've found that maintaining certain harmonic relationships between
 * frequencies for output on a single driver can greatly improve tactile
 * feel, so we recommend scaling down by octaves in these scenarios.
 *
 * @param freqData The frequency array to map.
 * @param dataLength The length of the frequency array.
 * @param exp The exponential value to apply to the normalized frequencies.
 */
void VibrosonicsAPI::mapFrequenciesExponential(float* freqData, int dataLength, float exp)
{
    float freqRatio;
    for (int i = 0; i < dataLength; i++) {
        if (freqData[i] <= 50) {
            continue;
        } // freq already within haptic range
        freqRatio = freqData[i] / (SAMPLE_RATE >> 1); // find where each freq lands in the spectrum
        freqData[i] = pow(freqRatio, exp) * 250; // convert into haptic range along a exp^ curve
    }
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
 * Calls update for every grain in the grain list
 */
void VibrosonicsAPI::updateGrains()
{
    Grain::update(&grainList);
}

/**
 * Updates an array of grains attack, sustain, and release windows if
 * the data's amplitude is greater than the amplitude stored in the grain.
 *
 * @param grains An array of grains to be triggered.
 * @param numGrains The size of the Grain array.
 * @param moduleData A pointer to a module's frequency and amplitude data
 */
void VibrosonicsAPI::triggerGrains(Grain* grains, int numGrains, float** moduleData)
{
    for (int i = 0; i < numPeaks; i++) {
        float currentAmp = moduleData[MP_AMP][i];
        float currentFreq = moduleData[MP_FREQ][i];
        if(moduleData[MP_AMP][i] > grains[i].getFrequency()){
            grains[i].setAttack(currentFreq, currentAmp, grains[i].getAttackDuration());
            grains[i].setSustain(currentFreq, currentAmp, grains[i].getSustainDuration());
            grains[i].setRelease(currentFreq, 0, grains[i].getReleaseDuration());
        }
    }
}

/**
 * Updates an array of grains parameters in the attack state.
 *
 * @param grains An array of grains
 * @param numGrains The size of a grain array
 * @param duration The number of windows the attack state will last for
 * @param freqMod The multiplicative modulation factor for the frequency of the attack state
 * @param ampMod The multiplicative modulation factor for the frequency of the attack state
 * @param curve Factor to influence the shave of the progression curve of the grain
 */
void VibrosonicsAPI::shapeGrainAttack(Grain* grains, int numGrains, int duration, float freqMod, float ampMod, float curve)
{
    for (int i = 0; i < numGrains; i++) {
        grains[i].shapeAttack(duration, freqMod, ampMod, curve);
    }
}

/**
 * Updates an array of grains parameters in the sustain state.
 *
 * @param grains An array of grains
 * @param numGrains The size of a grain array
 * @param duration The number of windows the sustain state will last for
 * @param freqMod The multiplicative modulation factor for the frequency of the sustain state
 * @param ampMod The multiplicative modulation factor for the frequency of the sustain state
 */
void VibrosonicsAPI::shapeGrainSustain(Grain* grains, int numGrains, int duration, float freqMod, float ampMod)
{
    for (int i = 0; i < numGrains; i++) {
        grains[i].shapeSustain(duration, freqMod, ampMod);
    }
}

/**
 * Updates an array of grains parameters in the release state.
 *
 * @param grains An array of grains
 * @param numGrains The size of a grain array
 * @param duration The number of windows the release state will last for
 * @param freqMod The multiplicative modulation factor for the frequency of the release state
 * @param ampMod The multiplicative modulation factor for the frequency of the release state
 * @param curve Factor to influence the shave of the progression curve of the grain
 */
void VibrosonicsAPI::shapeGrainRelease(Grain* grains, int numGrains, int duration, float freqMod, float ampMod, float curve)
{
    for (int i = 0; i < numGrains; i++) {
        grains[i].shapeRelease(duration, freqMod, ampMod, curve);
    }
}
