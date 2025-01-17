#include "VibrosonicsAPI.h"

void VibrosonicsAPI::init()
{
    AudioLab.init();
}

// performFFT feeds its input array to the ArduinoFFT fourier transform engine
// and stores the results in private data members vReal and vImag.
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

// storeFFTData writes the result of the most recent FFT computation into
// VibrosonicsAPI's circular buffer.
inline void VibrosonicsAPI::storeFFTData()
{
    circularBuffer.pushData((float*)vReal);
}

// finds and returns the mean amplitude
float VibrosonicsAPI::getMean(float* ampData, int dataLength)
{
    float sum = 0.0;
    for (int i = 0; i < dataLength; i++) {
        sum += ampData[i];
    }
    return sum > 0.0 ? sum / dataLength : sum;
}

// sets amplitude of bin to 0 if less than threshold
void VibrosonicsAPI::noiseFloor(float* ampData, float threshold)
{
    for (int i = 0; i < windowSizeBy2; i++) {
        if (ampData[i] < threshold) {
            ampData[i] = 0.0;
        }
    }
}

/**
 * Maps amplitudes in the input array to between 0-127 range.
 *
 * @param ampData The amplitude array to map.
 * @param dataLength The length of the amplitude array.
 * @param dataSumFloor The minimum value to normalize the amplitudes by (if
 * their sum is not greater than it).
 */
void VibrosonicsAPI::mapAmplitudes(float* ampData, int dataLength, float dataSumFloor)
{
    // sum amp data
    float dataSum = 0.0;
    for (int i = 0; i < dataLength; i++) {
        dataSum += ampData[i];
    }
    if (dataSum == 0.0)
        return; // return early if sum of amplitudes is 0

    // dataSumFloor is a special parameter that will need to be adjusted to find the max
    // dynamic contrast for a given set of ampllitudes. In general, a lower dataSumFloor will
    // proivde less dynamic contrast (i.e. the range of amplitudes will be compressed), whereas
    // a higher dataSumFloor may allow more room for contrast.
    // If the sum of amplitudes (dataSum) is larger than dataSumFloor then the dataSum will be used
    // to ensure values are no larger than 1.
    float divideBy = 1.0 / (dataSum > dataSumFloor ? dataSum : dataSumFloor);

    // convert amplitudes
    for (int i = 0; i < dataLength; i++) {
        ampData[i] = (ampData[i] * divideBy);
    }
}

// creates and adds a single dynamic wave to a channel with the given freq and amp
void VibrosonicsAPI::assignWave(float freq, float amp, int channel)
{
    Wave wave = AudioLab.dynamicWave(channel, freq, amp);
}

// creates and adds dynamic waves for a list of frequencies and amplitudes
void VibrosonicsAPI::assignWaves(float* freqData, float* ampData, int dataLength, int channel)
{
    for (int i = 0; i < dataLength; i++) {
        if (ampData[i] == 0.0 || freqData[i] == 0)
            continue; // skip storing if ampData is 0, or freqData is 0
        Wave wave = AudioLab.dynamicWave(channel, round(freqData[i]), ampData[i]); // create wave
    }
}

// processInput calls performFFT and storeFFTData to compute and store
// the FFT of AudioLab's input buffer.
void VibrosonicsAPI::processInput()
{
    performFFT(AudioLabInputBuffer);
    storeFFTData();
}

// runs doAnalysis on all added modules
void VibrosonicsAPI::analyze()
{
    // get data from circular buffer as 2D float array
    const float** data = (const float**)staticBuffer;

    // loop through added modules
    for (int i = 0; i < numModules; i++) {
        modules[i]->doAnalysis(data);
    }

    delete[] data;
}

// adds a new module to the Manager list of added modules
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
