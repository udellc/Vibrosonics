/*/
########################################################
  This code asynchronously samples a signal via an interrupt timer to fill an input buffer. Once the input buffer is full, a fast fourier transform is performed using the arduinoFFT library.
  The data from FFT is then processed and a new signal is formed using a sine wave table and audio synthesis, which fills an output buffer that is then outputted asynchronously by the same
  interrupt timer used for sampling the signal.

  Code written by: Mykyta "Nick" Synytsia - synytsim@oregonstate.edu
  
  Special thanks to: Vincent Vaughn, Chet Udell, and Oleksiy Synytsia
########################################################
/*/

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
#define AUD_OUT_PIN_L A0
#define AUD_OUT_PIN_R A1

#define FFT_WINDOW_SIZE 256   // size of a recording window, this value MUST be a power of 2
#define SAMPLING_FREQ 10000   // The audio sampling and audio output rate, with the ESP32 the fastest we could sample at is just under 24kHz, but
                              // bringing this down leaves more processing time for FFT, signal processing functions, and synthesis

#define FFT_MAX_AMP 300   // the maximum frequency magnitude of an FFT bin, this is multiplied by the total number of waves being synthesized for volume representation

#define MAX_NUM_PEAKS 8  // the maximum number of peaks to look for with the findMajorPeaks() function, also corresponds to how many sine waves are being synthesized

#define MAX_NUM_WAVES 8  // maximum number of waves to synthesize (all channels combined)
#define AUD_OUT_CH 2  // number of audio channels to synthesize

// #define DEBUG   // uncomment for debug mode, the time taken in loop is printed (in microseconds), along with the assigned sine wave frequencies and amplitudes

#if MAX_NUM_WAVES <= 0
#error MAX_NUM_WAVES 
#endif

#if AUD_OUT_CH <= 0
#error AUD_OUT_CH
#endif

/*/
########################################################
    Variables used for interrupt and function to call when interrupt is triggered
########################################################
/*/

static const int AUD_IN_BUFFER_SIZE = FFT_WINDOW_SIZE;  
static const int AUD_OUT_BUFFER_SIZE = FFT_WINDOW_SIZE * 2;

#ifdef USE_FFT
// used to store recorded samples for a gien window
volatile static int AUD_IN_BUFFER[AUD_IN_BUFFER_SIZE];
// rolling buffer for outputting synthesized signal
volatile static int AUD_OUT_BUFFER[AUD_OUT_CH][AUD_OUT_BUFFER_SIZE];
#endif

// audio input and output buffer index
volatile static int AUD_IN_BUFFER_IDX = 0;
volatile static int AUD_OUT_BUFFER_IDX = 0;

static hw_timer_t *SAMPLING_TIMER = NULL;

#ifdef DEBUG
static unsigned long loop_time;  // used for printing time main loop takes to execute in debug mode
#endif

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

        static constexpr float freqRes = float(SAMPLING_FREQ) / FFT_WINDOW_SIZE;  // the frequency resolution of FFT with the current window size

        static constexpr float freqWidth = 1.0 / freqRes; // the width of an FFT bin

        float freqs[FFT_WINDOW_SIZE_BY2];   // stores the data from the current fft window

        int FFTPeaks[FFT_WINDOW_SIZE_BY2 >> 1]; // stores the peaks computed by FFT
        float FFTPeaksAmp[FFT_WINDOW_SIZE_BY2 >> 1];
  
        static const int bassIdx = 200 * freqWidth; // FFT bin index of frequency ~200Hz

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
        struct wave 
        {
          int amp;
          int freq;
          int phase;
        };

        typedef struct wave Wave;

        // stores the sine wave frequencies and amplitudes to synthesize
        wave waves[AUD_OUT_CH][MAX_NUM_WAVES];


        // stores the number of sine waves in each channel
        int num_waves[AUD_OUT_CH];

        // stores the position of wave
        int sin_wave_idx = 0;

        // scratchpad array used for signal synthesis
        float generateAudioBuffer[AUD_OUT_CH][GEN_AUD_BUFFER_SIZE];
        // stores the current index of the scratch pad audio output buffer
        int generateAudioIdx = 0;
        // used for copying final synthesized values from scratchpad audio output buffer to volatile audio output buffer
        int generateAudioOutIdx = 0;

        /*/
        ########################################################
            Functions
        ########################################################
        /*/

        
        // calculate values for cosine function that is used for smoothing transition between frequencies and amplitudes (0.5 * (1 - cos((2PI / T) * x)), where T = AUD_OUT_BUFFER_SIZE
        void calculateWindowingWave();
        // calculate values for 1Hz sine wave @SAMPLING_FREQ sample rate
        void calculateWaves();

        /*/
        ########################################################
            Functions related to FFT analysis
        ########################################################
        /*/

        #ifdef USE_FFT
        // setup arrays for FFT analysis
        void setupFFT();

        // store the current window into freqs
        void storeFreqs();

        // noise flooring based on a set threshold
        void noiseFloor(float *data, float threshold);

        // finds all the peaks in the fft data* and removes the minimum peaks to contain output to @MAX_NUM_PEAKS
        void findMajorPeaks(float* data);

        // interpolation based on the weight of amplitudes around a peak
        int interpolateAroundPeak(float *data, int indexOfPeak);

        // maps amplitudes between 0 and 127 range to correspond to 8-bit (0, 255) DAC on ESP32 Feather
        void assignSinWaves(int* freqData, float* ampData, int size);
        void mapAmplitudes();
        #endif

        /*/
        ########################################################
            Functions related to generating values for the circular audio output buffer
        ########################################################
        /*/

        // returns value of sine wave at given frequency and amplitude
        float get_sin_wave_val(Wave w);

        // returns sum of sine waves of given channel
        float get_sum_of_channel(int ch);

        /*/
        ########################################################
            Functions related to sampling and outputting audio by interrupt
        ########################################################
        /*/

        // returns true if audio input buffer is full
        static bool AUD_IN_BUFFER_FULL();

        // restores AUD_IN_BUFFER_IDX, and ensures AUD_OUT_BUFFER is synchronized
        void RESET_AUD_IN_OUT_IDX();
        // outputs sample from AUD_OUT_BUFFER to DAC and reads sample from ADC to AUD_IN_BUFFER
        static void AUD_IN_OUT();

        // the function that is called when timer interrupt is triggered
        static void IRAM_ATTR ON_SAMPLING_TIMER();
        
    public:

        Vibrosonics();

        void init();

        bool ready();

        #ifdef USE_FFT
        void pullSamples();
        // pulls samples from audio input buffer, performs fft
        void performFFT();

        // FFT data processing
        void processData();
        #endif

        /*/
        ########################################################
            Wave assignment and modifcation
        ########################################################
        /*/

        // assigns a sine wave to specified channel, phase makes most sense to be between 0 and SAMPLING_FREQ where if phase == SAMPLING_FREQ / 2 then the wave is synthesized in counter phase
        void addSinWave(int freq, int amp, int ch, int phase = 0);
        // removes a sine wave at specified index and channel
        void removeSinWave(int idx, int ch);
        // modify sine wave at specified index and channel to desired frequency and amplitude
        void modifySinWave(int idx, int ch, int freq, int amp, int phase = 0);
        // sets all sine waves and frequencies to 0 on specified channel
        void resetSinWaves(int ch);
        // prints assigned sine waves
        void printSinWaves();

        // generates values for one window of audio output buffer
        void generateAudioForWindow(); 
    
};