#ifndef Vibrosonics_API_h
#define Vibrosonics_API_h
#include <arduinoFFT.h>
#include <AudioLab.h>
#include <AudioPrism.h>

// internal
#include "CircularBuffer.h"
#include "Grain.h"

// Analysis Module Flags
// NOTE: MOVE TO SEPERATE INCLUDE FILE AND CHANGE NAMES
#define MAJOR_PEAKS = 0;
#define SALIENT_FREQS = 1;
#define PERCUSSION_DETECTION = 2;
#define TOTAL_AMPLITUDE = 3;
#define NOISINESS = 4;
#define MEAN_AMPLITUDE = 5;
#define MAX_AMPLITUDE = 6;
#define DELTA_AMPLITUDES = 7;
#define FORMANTS = 8;
#define CENTROID = 9;
#define BREAD_SLICER= 10;

ArduinoFFT<float> FFT = ArduinoFFT<float>();

float vReal[WINDOW_SIZE];
float vImag[WINDOW_SIZE];

// --- AudioLab Library --------------------------------------------------------

// VibrosonicsAPI adopts AudioLab's input buffer
int* InputBuffer = AudioLab.getInputBuffer();


// --- AudioPrism Library ------------------------------------------------------

// AudioPrism modules can be registered with the Vibrosonics API using the
// VibrosonicsAPI::addModule() function. This array stores references to all
// AudioPrism modules registered with an instance of the API. This allows
// automatic synchronization of module's audio context (sample rate and
// window size) and allows performing simultaneous analysis on multiple
// modules with a single call to VibrosonicsAPI::analyze()

AnalysisModule modules;   // array of references to AudioPrism modules
int loadedModules = 0;

// --- Constants ---------------------------------------------------------------

// Some contants are defined to save repeated calculations
// WINDOW_SIZE and SAMPLE_RATE are defined in the AudioLab library

// WINDOW_SIZE_BY2: Half the window size.
// This constant is used often, becuase the output of a fast fourier
// transform has half as many values as the input window size.
static const int WINDOW_SIZE_BY2 = int(WINDOW_SIZE) >> 1;

// Frequency Resolution: The resolution is the frequency range, in Hz,
// represented by each output bin of the Fast Fourier Transform.
// Ex: 8192 Samples/Second / 256 Samples/Window = 32 Hz per output bin
const float frequencyResolution = float(SAMPLE_RATE) / WINDOW_SIZE;

// Frequency Width: The width is the duration of time represented by a
// single window's worth of samples.
// Ex: 256 Samples/Window / 8192 Samples/Second = 0.03125 Seconds/Window
const float frequencyWidth = float(WINDOW_SIZE) / frequencyResolution;

// --- FFT Input & Storage -----------------------------------------------------

// The circular buffer is used to efficiently store and retrieve multiple
// audio spectrums. Pushing new items on the circular buffer overwrites the
// oldest items, rather than shifting memory around.
// -- The first argument of the constructor is the number of audio spectrums
// to store. This is set to two because none of our analysis requires a
// longer history. This value can be increased arbitrarily to store longer
// histories.
// -- The second argument of the constructor is the number of bins in each
// audio spectrum. This will be the result of an FFT operation, so it must
// be set to half the window size used for the FFT.
// -- Note: AudioPrism modules take regular 2D float arrays as input. Call
// CircularBuffer::unwind to get a flat 2D version of the buffer's content.
float staticBuffer[2][WINDOW_SIZE_BY2];
CircularBuffer<float> circularBuffer = CircularBuffer((float *) staticBuffer, 2, WINDOW_SIZE_BY2);

// init
void init()
{
    Serial.begin(9600);
    while (!Serial)
    {
        delay(4000);
    }
    AudioLab.init();
}

//circular buffer operations

// performFFT feeds its input array to the ArduinoFFT fourier transform
// engine and stores the results in private data members vReal and vImag.
void performFFT()
{
    // copy samples from input to vReal and set vImag to 0
    for (int i = 0; i < WINDOW_SIZE; i++) {
        vReal[i] = InputBuffer[i];
        vImag[i] = 0.0;
    }

    // use arduinoFFT, 'FFT' object declared as private member of Vibrosonics
    FFT.dcRemoval(vReal, WINDOW_SIZE); // DC Removal to reduce noise
    FFT.windowing(vReal, WINDOW_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD); // Apply windowing function to data
    FFT.compute(vReal, vImag, WINDOW_SIZE, FFT_FORWARD); // Compute FFT
    FFT.complexToMagnitude(vReal, vImag, WINDOW_SIZE); // Compute frequency magnitudes

    circularBuffer.pushData((float*)vReal);
}

// runs doAnalysis on all added modules
void VibrosonicsAPI::analyze()
{
    // TODO: Do an actual loop through modules later
    modules[MAJOR_PEAKS]->doAnalysis(circularBuffer);
}


// Nick's function from randomFFTExample.ino
int interpolateAroundPeak(int indexOfPeak)
{
    float prePeak = indexOfPeak == 0 ? 0.0 : vReal[indexOfPeak - 1];
    float atPeak = vReal[indexOfPeak];
    float postPeak = indexOfPeak == WINDOW_SIZE_BY2 ? 0.0 : vReal[indexOfPeak + 1];
    float peakSum = prePeak + atPeak + postPeak;
    float magnitudeOfChange = ((atPeak + postPeak) - (atPeak + prePeak)) / (peakSum > 0.0 ? peakSum : 1);

    return int(round((float(indexOfPeak) + magnitudeOfChange) * frequencyResolution));
}

void createDynamicWave(int channel, float freq, float amp)
{
    AudioLab.dynamicWave(channel, freq, amp);
}

#endif // Vibrosonics_API_h
