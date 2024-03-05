#ifndef Vibrosonics_h
#define Vibrosonics_h

#include <AudioLab.h>

#define NUM_FREQ_WINDOWS 8  // the number of frequency windows to store in circular buffer freqs

#define FREQ_MAX_AMP_DELTA_MIN 10     // the min threshold of change in amplitude to be considered significant by the frequencyMaxAmplitudeDelta() function
#define FREQ_MAX_AMP_DELTA_MAX 500    // the max threshold of change in amplitude
#define FREQ_MAX_AMP_DELTA_K 8.0      // weight for amplitude of most change

class Vibrosonics
{

  private:

    static const int WINDOW_SIZE_BY2 = int(WINDOW_SIZE) >> 1;

    static const int MAX_NUM_PEAKS = WINDOW_SIZE_BY2 >> 1;
    int FFTPeaks[2][MAX_NUM_PEAKS];

  public:

    // this function has to be called once in Arduino's void setup(). 
    // Note: this is optional, stuff inside init() can be called directly inside setup()
    void init();

    // this is the function that has to be called in Arduino's void loop() (kind of like processData())
    // Note: this is optional, stuff inside update() can be called directly inside loop()
    void update();

    // performs FFT on data* and stores results in vReal array.
    float *performFFT(int* input); 

    float *getFFTData();

    float getSum(float *data, int dataLength);

    // returns the mean of some data
    float getMean(float* data, int dataLength);

    void doRMS(float *data, int dataLength, int size);

    // floors data that is below a certain threshold
    void noiseFloor(float *data, float threshold);

    void noiseFloorMean(float *data, int dataLength, float threshold);

    // floors data based on the data RMS multiplied by a threshold
    void smartNoiseFloor(float *data, int dataLength, float threshold, int rmsWindowSize);

    // looks for peaks and data and finds at most maxNumPeaks peaks
    void findPeaks(float* data, int maxNumPeaks);

    // maps amplitudes in some data to between 0-127 range
    void mapAmplitudes(float* ampData, int dataLength, float maxDataSum);

    // assigns sine waves based on the data
    void assignWaves(int* freqData, float* ampData, int dataLength, int channel, int startFrequency = 0, int endFrequency = 0);

    // interpolates a peak based on weight of surrounding values, optionally pass sampleRate and windowSize
    int interpolateAroundPeak(float *data, int indexOfPeak, int sampleRate = SAMPLE_RATE, int windowSize = WINDOW_SIZE);

    // returns the sum of the peak and surrounding values
    int sumOfPeak(float *data, int indexOfPeak);

    // finds the frequency of most change within minFreq and maxFreq, returns the index of the frequency of most change,
    // and stores the magnitude of change (between 0.0 and FREQ_MAX_AMP_DELTA_K) in magnitude reference
    int frequencyMaxAmplitudeDelta(float *data, float *prevData, int minFreq, int maxFreq, float &magnitude);


    // calculate sinc filter table
    // Parameter ratio is how many factors to downsample the signal by, i.e. if original sample rate
    // was 8192Hz and ratio is 4, the signal will be resampled to 2048Hz with a 1024Hz brick wall filter.
    // Parameter nz is the number of zero crossings in the filter, a higher nz value will result in a 
    // more ideal filter but will also considerably reduce performance
    void calculateDownsampleSincFilterTable(int ratio, int nz);

    // downsample a signal of length WINDOW_SIZE, returns true when downsampled output buffer is full
    // which will fill every @ratio windows
    // Note: data must be of length WINDOW_SIZE, and sample rate "does not" matter
    bool downsampleSignal(int* data);

    // returns pointer to downsampled signal buffer, call this when downsampleSignal() returns true
    int *getDownsampledSignal();

    // a very basic "testing" function to go through raw FFT data and return the frequency associated to
    //  the maximum bin based on the sampleRate
    int FFTMajorPeak(int sampleRate);

    void frequencyMapExample(float *frequencies, int numFrequencies, float fromMin, float fromMax, float toMin, float toMax, float aCurve);
    
};

#endif