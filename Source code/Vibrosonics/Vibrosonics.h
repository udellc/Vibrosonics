#ifndef Vibrosonics_h
#define Vibrosonics_h

#include <AudioLab.h>
#include "Analysis Modules/Analysis Module/AnalysisModule.h"

#define NUM_FREQ_WINDOWS 8  // the number of frequency windows to store in circular buffer freqs

#define FREQ_MAX_AMP_DELTA_MIN 50     // the min threshold of change in amplitude to be considered significant by the frequencyMaxAmplitudeDelta() function
#define FREQ_MAX_AMP_DELTA_MAX 500    // the max threshold of change in amplitude
#define FREQ_MAX_AMP_DELTA_K 16.0      // weight for amplitude of most change

class Vibrosonics
{

  private:

    static const int WINDOW_SIZE_BY2 = int(WINDOW_SIZE) >> 1;

    float freqs[NUM_FREQ_WINDOWS][WINDOW_SIZE_BY2];
    int freqsWindowCounter = 0;
    float* freqsCurrent = freqs[0];
    float* freqsPrevious = freqs[0];

    static const int MAX_NUM_PEAKS = WINDOW_SIZE_BY2 >> 1;
    int FFTPeaks[MAX_NUM_PEAKS];
    float FFTPeaksAmp[MAX_NUM_PEAKS];

    AnalysisModule** modules;

  public:

    // this function has to be called once in Arduino's void setup(). 
    // Note: this is optional, stuff inside init() can be called directly inside setup()
    void init();

    // this is the function that has to be called in Arduino's void loop() (kind of like processData())
    // Note: this is optional, stuff inside update() can be called directly inside loop()
    void update();

    // performs FFT on data* and stores results in vReal array.
    void performFFT(int* input); 

    // stores data computed by FFT into freqs array
    void storeFFTData();

    // returns the mean of some data
    float getMean(float* data, int dataLength);

    // floors data that is below a certain threshold
    void noiseFloor(float *data, float threshold);

    // looks for peaks and data and finds at most maxNumPeaks peaks
    void findPeaks(float* data, int maxNumPeaks);

    // maps amplitudes in some data to between 0-127 range
    void mapAmplitudes(float* ampData, int dataLength, float maxDataSum);

    // assigns sine waves based on the data
    void assignWaves(int* freqData, float* ampData, int dataLength);

    // interpolates a peak based on weight of surrounding values
    int interpolateAroundPeak(float *data, int indexOfPeak);

    // returns the sum of the peak and surrounding values
    int sumOfPeak(float *data, int indexOfPeak);

    // finds the frequency of most change within minFreq and maxFreq, returns the index of the frequency of most change, and stores the magnitude of change (between 0.0 and FREQ_MAX_AMP_DELTA_K) in magnitude reference
    int frequencyMaxAmplitudeDelta(float *data, float *prevData, int minFreq, int maxFreq, float &magnitude);
    
    void addModule(AnalysisModule* module);
    void processInput();
    void analyze();

};

#endif