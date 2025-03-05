#include "VibrosonicsAPI.h"

/**
 * Initializes all necessary api variables and dependencies.
 */
void VibrosonicsAPI::init()
{
    AudioLab.init();
}

/**
 * Feeds its input array to the ArduinoFFT fourier transform engine
 * and stores the results in private data members vReal and vImag.
 *
 * @param input Array of signals to do FFT on.
 */
void VibrosonicsAPI::performFFT(int* input)
{
    // copy samples from input to vReal and set vImag to 0
    for (int i = 0; i < WINDOW_SIZE; i++) {
        vReal[i] = input[i];
        vImag[i] = 0.0;
    }

    // use arduinoFFT, 'FFT' object declared as private member of Vibrosonics
    FFT.dcRemoval(vReal, WINDOW_SIZE); // DC Removal to reduce noise
    FFT.windowing(vReal, WINDOW_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD); // Apply windowing function to data
    FFT.compute(vReal, vImag, WINDOW_SIZE, FFT_FORWARD); // Compute FFT
    FFT.complexToMagnitude(vReal, vImag, WINDOW_SIZE); // Compute frequency magnitudes
}

float* VibrosonicsAPI::getCurrentWindow() const
{
    return spectrogram.getCurrentWindow();
}

float* VibrosonicsAPI::getPreviousWindow() const
{
    return spectrogram.getPreviousWindow();
}

float* VibrosonicsAPI::getWindowAt(int relativeIndex) const
{
    return spectrogram.getWindowAt(relativeIndex);
}

/**
 * Finds and returns the mean value of data.
 *
 * @param ampData Array of amplitudes.
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

    // copy data to sample from
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
 * Calls performFFT to compute the FFT of AudioLab's input buffer. Stores the
 * result in the spectrogram. Should be called each time AudioLab is ready.
 */
void VibrosonicsAPI::processInput()
{
    performFFT(AudioLabInputBuffer);
    spectrogram.pushWindow(vReal);
}

/**
 * Calls performFFT to compute the FFT of AudioLab's input buffer. Floors
 * noise with a constant threshold and stores the result in the spectrogram.
 * Should be called each time AudioLab is ready.
 *
 * @param noiseThreshold The amplitude cutoff value.
 */
void VibrosonicsAPI::processInput(float noiseThreshold)
{
    performFFT(AudioLabInputBuffer);
    noiseFloor(vReal, WINDOW_SIZE_BY_2, noiseThreshold);
    spectrogram.pushWindow(vReal);
}

/**
 * Calls performFFT to compute the FFT of AudioLab's input buffer. Floors
 * noise using the constant false alarm rate (CFAR) and stores the result in
 * the spectrogram. Should be called each time AudioLab is ready.
 *
 * @param numRefs The number of reference cells for CFAR.
 * @param numGuards The number of guard cells for CFAR.
 * @param bias The bias factor to use for CFAR.
 */
void VibrosonicsAPI::processInput(int numRefs, int numGuards, float bias)
{
    performFFT(AudioLabInputBuffer);
    noiseFloorCFAR(vReal, WINDOW_SIZE_BY_2, numRefs, numGuards, bias);
    spectrogram.pushWindow(vReal);
}

/**
 * Calls the doAnalysis function for each loaded module in the modules array.
 */
void VibrosonicsAPI::analyze()
{
    // loop through added modules
    for (int i = 0; i < numModules; i++) {
        modules[i]->doAnalysis();
    }
}

/**
 * Adds a new module to the modules array. The module must be created and
 * passed by the caller.
 *
 * @param module Module to be added to the modules array.
 */
void VibrosonicsAPI::addModule(AnalysisModule* module)
{
    module->setWindowSize(WINDOW_SIZE);
    module->setSampleRate(SAMPLE_RATE);

    // create new larger array for modules
    numModules++;
    AnalysisModule** newModules = new AnalysisModule*[numModules];

    // copy modules over and add new module
    for (int i = 0; i < numModules - 1; i++) {
        newModules[i] = modules[i];
    }
    newModules[numModules - 1] = module;

    // free old modules array and store reference to new modules array
    delete[] modules;
    modules = newModules;
}

/**
 * Adds a new module to the modules array. The module must be created and
 * passed by the caller.
 *
 * @param module Module to be added to the modules array.
 */
void VibrosonicsAPI::addModule(AnalysisModule* module, int lowerFreq, int upperFreq)
{
    module->setWindowSize(WINDOW_SIZE);
    module->setSampleRate(SAMPLE_RATE);
    module->setAnalysisRangeByFreq(lowerFreq, upperFreq);

    // create new larger array for modules
    numModules++;
    AnalysisModule** newModules = new AnalysisModule*[numModules];

    // copy modules over and add new module
    for (int i = 0; i < numModules - 1; i++) {
        newModules[i] = modules[i];
    }
    newModules[numModules - 1] = module;

    // free old modules array and store reference to new modules array
    delete[] modules;
    modules = newModules;
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
 * Creates a singular grain with specified channel and wave type and then pushes the
 * newly created grain to the global grain list.
 *
 * @param channel The physical speaker channel, on current hardware valid inputs are 0-2
 * @param waveType The type of wave Audiolab will generate utilizing the grains.
 */

Grain* VibrosonicsAPI::createGrain(uint8_t channel, WaveType waveType)
{
    return createGrainArray(1, channel, waveType);
}

/**
 * Calls update for every grain in the grain list
 */
void VibrosonicsAPI::updateGrains()
{
    Grain::update(&grainList);
}

/**
 * Updates an array of numPeaks grains sustain and release windows if
 * the data's amplitude is greater than the amplitude stored in the grain.
 *
 * @param numPeaks The size of the Grain array.
 * @param peakData A pointer to a module's amplitude data
 * @param grains An array of grains to be triggered.
 */

void VibrosonicsAPI::triggerGrains(Grain* grains, int numPeaks, float** peakData)
{
    // NOTE: ADD FREQUENCY MATCHING
    // Plan: First pass, check to see if incoming peakData frequencies match previous grain frequencies.
    // So if previous grains are [200hz, 400hz, 800hz, 1600hz], and incoming data is [400hz, 800hz, 1600hz, 320hz]
    // the grain mapping will be [320hz, 400hz, 800hz, 1600hz]. Basically make sure that each grain is actually keeping as similair a frequency as possible
    // Keep the data in sync
    // issue: the Just noticeable difference for people in this frequency range is 1hz. gonna have to match basically exactly.

    float sortedFreqs[numPeaks];
    float sortedAmps[numPeaks];

    for (int i = 0; i < numPeaks; i++) {
        sortedFreqs[i] = peakData[MP_FREQ][i];
        sortedAmps[i] = peakData[MP_AMP][i];
    }

    for (int i = 0; i < numPeaks - 1; i++) {
        for (int j = 0; j < numPeaks - i - 1; j++) {
            if (sortedFreqs[j] > sortedFreqs[j + 1]) {
                float tempFreq = sortedFreqs[j];
                sortedFreqs[j] = sortedFreqs[j + 1];
                sortedFreqs[j + 1] = tempFreq;

                float tempAmp = sortedAmps[j];
                sortedAmps[j] = sortedAmps[j + 1];
                sortedAmps[j + 1] = tempAmp;
            }
        }
    }

    for (int i = 0; i < numPeaks - 1; i++) {
        for (int j = 0; j < numPeaks - i - 1; j++) {
            if (grains[j].getFrequency() > grains[j + 1].getFrequency()) {
                Grain temp = grains[j];
                grains[j] = grains[j + 1];
                grains[j + 1] = temp;
            }
        }
    }

    for (int i = 0; i < numPeaks; i++) {
        float closestFreq = sortedFreqs[i];
        float closestAmp = sortedAmps[i];

        if (closestAmp >= grains[i].getAmplitude()) {
            grains[i].setAttack(closestFreq, closestAmp, grains[i].getAttackDuration());
            grains[i].setSustain(closestFreq, closestAmp, grains[i].getSustainDuration());
            grains[i].setRelease(closestFreq, 0, grains[i].getReleaseDuration());
        }
    }
}

void VibrosonicsAPI::shapeGrainAttack(Grain* grains, int numGrains, int duration, float freqMod, float ampMod, float curve)
{
    for (int i = 0; i < numGrains; i++) {
        grains[i].shapeAttack(duration, freqMod, ampMod, curve);
    }
}

void VibrosonicsAPI::shapeGrainSustain(Grain* grains, int numGrains, int duration, float freqMod, float ampMod)
{
    for (int i = 0; i < numGrains; i++) {
        grains[i].shapeSustain(duration, freqMod, ampMod);
    }
}

void VibrosonicsAPI::shapeGrainRelease(Grain* grains, int numGrains, int duration, float freqMod, float ampMod, float curve)
{
    for (int i = 0; i < numGrains; i++) {
        grains[i].shapeRelease(duration, freqMod, ampMod, curve);
    }
}
