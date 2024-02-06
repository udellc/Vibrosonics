#include <arduinoFFTFloat.h>
#include "AudioLab.h"

class Vibrosonics
{
    int sampleRate = 8192;
    
    int windowSize = 256;
    int windowSizeBy2 = windowSize >> 1;
    
    int numFreqWindows = 8;
    float frequencyResolution = float(sampleRate) / windowSize;
    float frequencyWidth = 1.0 / frequencyResolution;
    
    // FFT
    float* vReal;
    float* vImag;
    arduinoFFT FFT = arduinoFFT();

    float freqs[numFreqWindows][windowSize>>1];
    int freqsWindowCounter = 0;
    float* freqsCurrent = freqs[0];
    float* freqsPrevious = freqs[0];

    //int* AudioLabInputBuffer = AudioLab.getInputBuffer();

    // OPTION 1: composition
    // generators contain analysis modules
    // vibrosonics contains generators

    // OPTION 2: interface
    // interface contains generators and analysis modules
    // vibrosonics contains interfaces

    // OPTION 3: two arrays
    // positive: vibrosonics has easy access to modules and generators

    AnalysisModule* modules;
    Generator* generators;
};

{
    v.process()
}

Vibrosonics::Vibrosonics()
{   

}

Vibrosonics::Vibrosonics(int winSize, int sampRate)
{   

}

Vibrosonics::update()
{
    // input -> FFT
    // FFT -> modules
    // modules -> generators
    // generators -> output
}
