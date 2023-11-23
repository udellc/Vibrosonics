/*/
########################################################
  This code asynchronously samples a signal via an interrupt timer to fill an input buffer. Once the input buffer is full, a fast fourier transform is performed using the arduinoFFT library.
  The data from FFT is then processed and a new signal is formed using a sine wave table and audio synthesis, which fills an output buffer that is then outputted asynchronously by the same
  interrupt timer used for sampling the signal.

  Code written by: Mykyta "Nick" Synytsia - synytsim@oregonstate.edu
  
  Special thanks to: Vincent Vaughn, Chet Udell, and Oleksiy Synytsia
########################################################
/*/

#ifndef Vibrosonics_h
#define Vibrosonics_h

#include <arduinoFFTFloat.h>
#include <Arduino.h>
#include <driver/dac.h>
#include <driver/adc.h>
#include <math.h>

/*/
########################################################
  Directives
########################################################
/*/

#define USE_FFT  // comment to disable FFT

// audio sampling stuff
#define AUD_IN_PIN ADC1_CHANNEL_6 // corresponds to ADC on A2
// #define AUD_IN_PINA A2
#define AUD_OUT_PIN_L A0
#define AUD_OUT_PIN_R A1

#define FFT_WINDOW_SIZE 256   // size of a recording window, this value MUST be a power of 2
#define SAMPLING_FREQ 10000   // The audio sampling and audio output rate, with the ESP32 the fastest we could sample at is just under 24kHz, but
                // bringing this down leaves more processing time for FFT, signal processing functions, and synthesis

#define FFT_MAX_AMP 300   // the maximum frequency magnitude of an FFT bin, this is multiplied by the total number of waves being synthesized for volume representation

#define NUM_FREQ_WINDOWS 8  // the number of frequency windows to store in circular buffer freqs

#define FREQ_MAX_AMP_DELTA_MIN 50     // the min threshold of change in amplitude to be considered significant by the frequencyMaxAmplitudeDelta() function
#define FREQ_MAX_AMP_DELTA_MAX 500    // the max threshold of change in amplitude
#define FREQ_MAX_AMP_DELTA_K 16.0      // weight for amplitude of most change

#define MAX_NUM_PEAKS 7  // the maximum number of peaks to look for with the findMajorPeaks() function

#define MAX_NUM_WAVES 8  // maximum number of waves to synthesize (all channels combined)
#define NUM_OUT_CH 2  // number of audio channels to synthesize

#define MIRROR_MODE

#define DEBUG   // uncomment for debug mode, the time taken in loop is printed (in microseconds), along with the assigned sine wave frequencies and amplitudes

/*/
########################################################
  Check for errors in directives
########################################################
/*/

#if MAX_NUM_WAVES <= 0
#error MAX_NUM_WAVES MUST BE AT LEAST 1
#endif

#if NUM_OUT_CH <= 0
#error NUM_OUT_CH MUST BE AT LEAST 1
#endif

/*/
########################################################
  Variables used for interrupt and function to call when interrupt is triggered
########################################################
/*/

static const int AUD_IN_BUFFER_SIZE = FFT_WINDOW_SIZE;  
static const int AUD_OUT_BUFFER_SIZE = FFT_WINDOW_SIZE * 2;

// used to store recorded samples for a gien window
volatile static int AUD_IN_BUFFER[AUD_IN_BUFFER_SIZE];
// rolling buffer for outputting synthesized signal
volatile static int AUD_OUT_BUFFER[NUM_OUT_CH][AUD_OUT_BUFFER_SIZE];

// audio input and output buffer index
volatile static int AUD_IN_BUFFER_IDX = 0;
volatile static int AUD_OUT_BUFFER_IDX = 0;

static hw_timer_t *SAMPLING_TIMER = NULL;

// class declaration

class Vibrosonics
{
  
  private:

    /*/
    ########################################################
      FFT
    ########################################################
    /*/

    #ifdef USE_FFT
    float vReal[FFT_WINDOW_SIZE];  // vReal is used for input at SAMPLING_FREQ and receives computed results from FFT
    float vImag[FFT_WINDOW_SIZE];  // used to store imaginary values for computation

    arduinoFFT FFT = arduinoFFT(vReal, vImag, FFT_WINDOW_SIZE, SAMPLING_FREQ);  // Object for performing FFT's


    /*/
    ########################################################
      Stuff relevant to FFT signal processing
    ########################################################
    /*/

    static const int sampleDelayTime = 1000000 / SAMPLING_FREQ;

    // FFT_SIZE_BY_2 is FFT_WINDOW_SIZE / 2. We are sampling at double the frequency we are trying to detect, therefore only half of the FFT bins are used for analysis post FFT
    static const int FFT_WINDOW_SIZE_BY2 = int(FFT_WINDOW_SIZE) >> 1;

    static const constexpr float freqRes = float(SAMPLING_FREQ) / FFT_WINDOW_SIZE;  // the frequency resolution of FFT with the current window size

    static const constexpr float freqWidth = 1.0 / freqRes; // the width of an FFT bin

    float freqs[NUM_FREQ_WINDOWS][FFT_WINDOW_SIZE_BY2];   // stores frequency magnitudes computed by FFT over NUM_FREQ_WINDOWS
    int freqsWindow = 0;                                  // time position of freqs buffer
    int freqsWindowPrev = 0;

    float* freqsCurrent = NULL;
    float* freqsPrevious = NULL;

    int FFTPeaks[FFT_WINDOW_SIZE_BY2 >> 1]; // stores the peaks computed by FFT
    float FFTPeaksAmp[FFT_WINDOW_SIZE_BY2 >> 1];
  
    static const int bassIdx = 120 * freqWidth; // FFT bin index of frequency ~200Hz

    static const int MAX_AMP_SUM = MAX_NUM_WAVES * FFT_MAX_AMP;
    #endif

    /*/
    ########################################################
      Variables used for generating an audio output
    ########################################################
    /*/

    static const int GEN_AUD_BUFFER_SIZE = FFT_WINDOW_SIZE * 3;

    const float _SAMPLING_FREQ = 1.0 / SAMPLING_FREQ;

    // a cosine wave for modulating sine waves
    float cos_wave_w[AUD_OUT_BUFFER_SIZE];

    // sine wave for storing pre-generated values of a sine wave at SAMPLING_FREQ sample rate
    float sin_wave[SAMPLING_FREQ];
    // float cos_wave[SAMPLING_FREQ];
    // float tri_wave[SAMPLING_FREQ];
    // float sqr_wave[SAMPLING_FREQ];
    // float saw_wave[SAMPLING_FREQ];

    // structure for wave
    typedef struct wave 
    {
      wave(int amp, int freq, int phase): freq(freq), amp(amp), phase(phase) {}
      wave(): amp(0), freq(0), phase(0) {}
      int freq;
      int amp;
      int phase;
    };

    typedef struct wave_map
    {
      wave_map(int ch, int amp, int freq, int phase): ch(ch), valid(1), w(wave(freq, amp, phase)) {}
      wave_map(): ch(0), valid(0), w(wave(0, 0, 0)) {}
      int ch = 0;
      int idx = 0;
      bool valid = 0;
      wave w;
    }; 

    wave_map waves_map[MAX_NUM_WAVES];

    // stores the sine wave frequencies and amplitudes to synthesize
    wave waves[NUM_OUT_CH][MAX_NUM_WAVES];

    // stores the number of sine waves in each channel
    int num_waves[NUM_OUT_CH];

    // stores the position of wave
    int sin_wave_idx = 0;

    // scratchpad array used for signal synthesis
    float generateAudioBuffer[NUM_OUT_CH][GEN_AUD_BUFFER_SIZE];
    // stores the current index of the scratch pad audio output buffer
    int generateAudioIdx = 0;
    // used for copying final synthesized values from scratchpad audio output buffer to volatile audio output buffer
    int generateAudioOutIdx = 0;

    /*/
    ########################################################
      Functions related to FFT analysis
    ########################################################
    /*/

    #ifdef USE_FFT
    // store the current window into freqs array with freqsWindow acting as time
    void storeFreqs();

    // noise flooring based on a set threshold
    void noiseFloor(float *data, float threshold);

    // returns mean of data
    float getMean(float *data, int size);

    // finds the frequency of most change within minFreq and maxFreq, returns the index of the frequency of most change, and stores the magnitude of change (between 0.0 and FREQ_MAX_AMP_DELTA_K) in magnitude reference
    int frequencyMaxAmplitudeDelta(float *data, float *prevData, int minFreq, int maxFreq, float &magnitude);

    // finds all the peaks in the fft data* and removes the minimum peaks to contain output to @MAX_NUM_PEAKS
    void findMajorPeaks(float* data);

    // interpolation based on the weight of amplitudes around a peak
    int interpolateAroundPeak(float *data, int indexOfPeak);

    // returns the sum around an index in data, helps better represent the energy of a peak
    int sumOfPeak(float *data, int indexOfPeak);

    // assigns passed freq and amp data to waves
    void assignWaves(int* freqData, float* ampData, int size);

    // weights frequencies above 120Hz to between 0 and 120Hz
    int freqWeighting(int freq);
    #endif

    /*/
    ########################################################
      Functions related to mapping waves to id
    ########################################################
    /*/

    // returns true if id is valid
    bool isValidId(int id);
    // returns true if channel exists
    bool isValidChannel(int ch);


    /*/
    ########################################################
      Functions related to generating values for the circular audio output buffer
    ########################################################
    /*/

    // calculate values for cosine function that is used for smoothing transition between frequencies and amplitudes (0.5 * (1 - cos((2PI / T) * x)), where T = AUD_OUT_BUFFER_SIZE
    void calculate_windowing_wave();
    // calculate values for 1Hz sine wave @SAMPLING_FREQ sample rate
    void calculate_waves();
    // returns value of sine wave at given frequency and amplitude
    float get_wave_val(wave w);
    // returns sum of sine waves of given channel
    float get_sum_of_channel(int ch);

    // maps amplitudes between 0 and 127 range to correspond to 8-bit (0, 255) DAC on ESP32 Feather
    void mapAmplitudes(int minSum = MAX_AMP_SUM);

    /*/
    ########################################################
      Functions related to sampling and outputting audio by interrupt
    ########################################################
    /*/

    void setupISR();

    void initAudio();

    // returns true if audio input buffer is full
    static bool IRAM_ATTR AUD_IN_BUFFER_FULL();

    // restores AUD_IN_BUFFER_IDX, and ensures AUD_OUT_BUFFER is synchronized
    void RESET_AUD_IN_OUT_IDX();

    // outputs sample from AUD_OUT_BUFFER to DAC and reads sample from ADC to AUD_IN_BUFFER
    static void IRAM_ATTR AUD_IN_OUT();

    // the function that is called when timer interrupt is triggered
    static void IRAM_ATTR ON_SAMPLING_TIMER();

    // static int IRAM_ATTR local_adc1_read(adc1_channel_t channel);
    
  public:
    // class init
    Vibrosonics();

    // initialize pins and setup interrupt timer
    void init();
    // returns true when the audio input buffer fills. If not using FFT then returns true when modifications to waves can be made
    bool ready();
    // call resume to continue audio input and/or output
    void resume();

    #ifdef USE_FFT
    // pulls samples from audio input buffer and stores them directly to vReal
    void pullSamples();
    // pulls samples from audio input buffer and stores them directly to a specified output buffer
    void pullSamples(float *output);
    // performs fft on vReal buffer
    void performFFT();
    // perform FFT on a given input (SIZE OF INPUT MUST BE EQUAL TO WINDOW SIZE)
    void performFFT(float *input);

    // Process data from FFT
    void processData();
    #endif

    /*/
    ########################################################
      Wave assignment and modifcation
    ########################################################
    /*/

    // assigns a sine wave to specified channel, phase makes most sense to be between 0 and SAMPLING_FREQ where if phase == SAMPLING_FREQ / 2 then the wave is synthesized in counter phase
    int addWave(int ch, int freq, int amp, int phase = 0);
    // removes a wave given id
    void removeWave(int id);
    // modify wave given id
    void modifyWave(int id, int freq, int amp, int phase = 0, int ch = 0);
    // sets all sine waves and frequencies to 0 on specified channel
    void resetWaves(int ch);
    // resets waves on all channels
    void resetAllWaves();

    // returns total number of waves on a ch, returns -1 if passed channel is invalid
    int getNumWaves(int ch);
    // returns frequency of wave id, returns -1 if id is invalid
    int getFreq(int id);
    // returns amplitude of wave id, returns -1 if id is invalid
    int getAmp(int id);
    // returns phase of wave id, returns -1 if id is invalid
    int getPhase(int id);
    // returns channel of wave id
    int getChannel(int id);

    // change frequency of wave with given id, returns true if frequency was set
    bool setFreq(int id, int freq);
    // change amplitude of wave with given id, returns true if amplitude was set
    bool setAmp(int id, int amp);
    // change phase of wave with given id, returns true if phase was set
    bool setPhase(int id, int phase);
    // set channel of wave with given id, returns true if channel was set
    bool setChannel(int id, int ch);

    // generates values for one window of audio output buffer
    void generateAudioForWindow(); 

    // prints assigned sine waves
    void printWaves();

    // prints assigned sine waves
    void printWavesB();

    // enable interrupt timer
    void enableAudio();
    // disable interrupt timer
    void disableAudio();
  
};

class GenerateAudio
{
  private:

  public:
};

#endif