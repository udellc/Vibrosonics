#include "VibrosonicsAPI.h"

void VibrosonicsAPI::init() 
{
  Serial.begin(115200);
  while (!Serial)
    ;
  delay(4000);

  AudioLab.init();
}

// performFFT feeds its input array to the ArduinoFFT fourier transform engine 
// and stores the results in private data members vReal and vImag.
void VibrosonicsAPI::performFFT(int *input) {
    // copy samples from input to vReal and set vImag to 0
    for (int i = 0; i < WINDOW_SIZE; i++) {
        vReal[i] = input[i];
        vImag[i] = 0.0;
    }

    // use arduinoFFT, 'FFT' object declared as private member of Vibrosonics
    FFT.dcRemoval(vReal, WINDOW_SIZE);                                    // DC Removal to reduce noise
    FFT.windowing(vReal, WINDOW_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
    FFT.compute(vReal, vImag, WINDOW_SIZE, FFT_FORWARD);                  // Compute FFT
    FFT.complexToMagnitude(vReal, vImag, WINDOW_SIZE);                    // Compute frequency magnitudes
}

// storeFFTData writes the result of the most recent FFT computation into 
// VibrosonicsAPI's circular buffer.
inline void VibrosonicsAPI::storeFFTData() {
    buffer.write(vReal, frequencyWidth);
}

// finds and returns the mean amplitude
float VibrosonicsAPI::getMean(float *ampData, int dataLength) {
    float sum = 0.0;
    for (int i = 0; i < dataLength; i++) {
        sum += ampData[i];
    }
    return sum > 0.0 ? sum / dataLength : sum;
}

// sets amplitude of bin to 0 if less than threshold
void VibrosonicsAPI::noiseFloor(float *ampData, float threshold) {
    for (int i = 0; i < WINDOW_SIZE_BY2; i++) {
        if (ampData[i] < threshold) {
            ampData[i] = 0.0;
        }
    }
}

// maps and normalizes amplitudes. Necessary for use with AudioLab.
void VibrosonicsAPI::mapAmplitudes(float* ampData, int dataLength, float maxDataSum) {
    // sum amp data
    float dataSum = 0.0;
    for (int i = 0; i < dataLength; i++) {
        dataSum += ampData[i];
    }
    if (dataSum == 0.0) return;  // return early if sum of amplitudes is 0

    // maxDataSum is a special parameter that will need to be adjusted to find the max 
    // dynamic contrast for a given set of ampllitudes. In general, a lower maxDataSum will 
    // proivde less dynamic contrast (i.e. the range of amplitudes will be compressed), whereas
    // a higher maxDataSum may allow more room for contrast.
    // If the sum of amplitudes (dataSum) is larger than maxDataSum, then the dataSum will be used
    // to ensure values are no larger than 1.
    float divideBy = 1.0 / (dataSum > maxDataSum ? dataSum : maxDataSum);

    // convert amplitudes
    for (int i = 0; i < dataLength; i++) {
        ampData[i] = (ampData[i] * divideBy);
    }
}

// creates and adds a single dynamic wave to a channel with the given freq and amp
void VibrosonicsAPI::assignWave(float freq, float amp, int channel) {
    Wave wave = AudioLab.dynamicWave(channel, freq, amp); 
}

// creates and adds dynamic waves for a list of frequencies and amplitudes
void VibrosonicsAPI::assignWaves(float* freqData, float* ampData, int dataLength, int channel) {
    for (int i = 0; i < dataLength; i++) {
        if (ampData[i] == 0.0 || freqData[i] == 0) continue; // skip storing if ampData is 0, or freqData is 0
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
    const float** data = buffer.unwind();

    // loop through added modules
    for(int i=0; i<numModules; i++)
    {
        modules[i]->doAnalysis(data);
    }

    delete [] data;
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
    for(int i=0; i<numModules-1; i++){
        newModules[i] = modules[i];
    }
    newModules[numModules-1] = module;
    
    // free old modules array and store reference to new modules array
    delete [] modules;
    modules = newModules;
}

// mapFrequenciesLinear() and mapFrequencyExponential() are basic mapping functions
// to convert a list of frequencies from 0-SAMPLE_RATE to the haptic range (0-250).
// In our experience, these functions can help reduce high pitch 'R2-D2' noises in output.
// However, it has been found that maintaining the harmonic relationships of frequencies
// can better describe music, so we recommend scaling down by octaves in these scenarios.
void VibrosonicsAPI::mapFrequenciesLinear(float* freqData, int dataLength)
{
    float freqRatio;
    // loop through frequency data
    for (int i = 0; i < dataLength; i++) {
        freqRatio = freqData[i] / (SAMPLE_RATE>>1); // find where each freq lands in the spectrum
        freqData[i] = round(freqRatio * 250) + 20;  // convert into to haptic range linearly
    }
}

// maps frequencies from 0-SAMPLE_RATE to haptic range (0-250Hz) using an exponent 
void VibrosonicsAPI::mapFrequenciesExponential(float* freqData, int dataLength, float exp)
{
    float freqRatio;
    for (int i=0; i < dataLength; i++) {
        if (freqData[i] <= 50) { continue; }        // freq already within haptic range
        freqRatio = freqData[i] / (SAMPLE_RATE>>1); // find where each freq lands in the spectrum
        freqData[i] = pow(freqRatio, exp) * 250;    // convert into haptic range along a exp^ curve
    }
}
