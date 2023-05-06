#include <arduinoFFT.h>
#include <Arduino.h>
#include <math.h>

/*/
########################################################
  Directives
########################################################
/*/

#define AUDIO_INPUT_PIN A2
#define AUDIO_OUTPUT_PIN A0

#define FFT_WINDOW_SIZE 256   // Number of FFT windows, this value must be a power of 2
#define SAMPLING_FREQ 8192    // The audio sampling and audio output rate, with the ESP32 the fastest we could sample at is just above 10kHz, but
                              // bringing this down to just a tad lower, leaves more room for FFT and other signal processing functions to be done

#define FFT_FLOOR_THRESH 500  // amplitude flooring threshold for FFT, to reduce noise
#define FFT_OUTLIER 30000     // Outlier for unusually high amplitudes in FFT

#define BREADSLICER_NUM_SLICES 5 // The number of slices the breadslicer does
#define BREADSLICER_CURVE_EXPONENT 3.0  // The exponent for the used for the breadslicer curve
#define BREADSLICER_CURVE_OFFSET 0.59  // The curve offset for the breadslicer to follow when slicing the amplitude array
#define BREADSLICER_MAX_AVG_BIN 2000 // The minimum max that is used for scaling the amplitudes to the 0-255 range, and helps represent the volume

#define SINE_WAVE_TABLE_SIZE 40  // How many frequencies to pre-generate (i.e from 1 to SIN_WAVE_TABLE_SIZE). This is used to speed up calculations performed by generateAudioForWindow()

#define AUDIO_INPUT_BUFFER_SIZE int(FFT_WINDOW_SIZE)
#define AUDIO_OUTPUT_BUFFER_SIZE int(FFT_WINDOW_SIZE * 3)

/*/
########################################################
  FFT
########################################################
/*/

const int sampleDelayTime = int(1000000 / SAMPLING_FREQ); // the time per sample 

float vReal[FFT_WINDOW_SIZE];  // vRealL is used for input from the left channel and receives computed results from FFT
float vImag[FFT_WINDOW_SIZE];  // vImagL is used to store imaginary values for computation

arduinoFFT FFT = arduinoFFT(vReal, vImag, FFT_WINDOW_SIZE, SAMPLING_FREQ);  // Object for performing FFT's

/*/
########################################################
  Stuff relevant to FFT signal processing
########################################################
/*/

const int SAMPLING_FREQ_BY2 = SAMPLING_FREQ / 2.0;  // the highest theoretical frequency that we can detect, since we need to sample twice the frequency we are trying to detect

// FFT_SIZE_BY_2 is FFT_WINDOW_SIZE / 2. We are sampling at double the frequency we are trying to detect, therefore only half of the vReal array is used for analysis post FFT
const int FFT_WINDOW_SIZE_BY2 = int(FFT_WINDOW_SIZE) >> 1;

const int frequencyResolution = SAMPLING_FREQ / FFT_WINDOW_SIZE;  // the frequency resolution of FFT with the current window size

float vRealPrev[FFT_WINDOW_SIZE_BY2]; // stores the previous amplitudes calculated by FFT of the left channel, for averaging windows

float FFTData[FFT_WINDOW_SIZE_BY2];   // stores the data the signal processing functions will use for the left channel

const int slices = BREADSLICER_NUM_SLICES;

float amplitudeToRange = (4095.0/BREADSLICER_MAX_AVG_BIN) / slices;   // the "K" value used to put the average amplitudes calculated by breadslicer into the 0-255 range. This value alters but doesn't go below BREADSLICER_MAX_AVG_BIN to give a sense of the volume

int breadslicerSliceLocations[slices];
float breadslicerSliceWeights[slices] = {0.6, 2.0, 2.5, 2.0, 2.0};

int averageAmplitudeOfSlice[slices];          // the array used to store the average amplitudes calculated by the breadslicer
int peakFrequencyOfSlice[slices];             // the array containing the peak frequency of the slice

volatile int numFFTCount = 0;

/*/
########################################################
  Variables used for generating an audio output
########################################################
/*/


const int FFT_WINDOW_SIZE_X2 = FFT_WINDOW_SIZE * 2;

float resolution = float(2.0 * PI / FFT_WINDOW_SIZE_X2);

int generateAudioCounter = 0;

int toggleOffsetWave = 0;

float cos_wave[FFT_WINDOW_SIZE_X2];

// sine wave table for storing pre-generated values of sine waves from 1 to 200Hz (integer)
int sin_wave_table[SINE_WAVE_TABLE_SIZE][FFT_WINDOW_SIZE_X2];

/*/
########################################################
  Variables used for interrupt and function to call when interrupt is triggered
########################################################
/*/

volatile int audioInputBuffer[AUDIO_OUTPUT_BUFFER_SIZE];
volatile int audioInputBufferCount = 0;
volatile int audioInputBufferFull = 0;

volatile int audioOutputBuffer[AUDIO_OUTPUT_BUFFER_SIZE];
volatile int audioOutputBufferCount = 0;

hw_timer_t *My_timer = NULL;

void IRAM_ATTR onTimer(){
  analogWrite(AUDIO_OUTPUT_PIN, audioOutputBuffer[audioOutputBufferCount]);
  if (numFFTCount > 2) {
    audioOutputBufferCount = (audioOutputBufferCount + 1) % AUDIO_OUTPUT_BUFFER_SIZE;
  }
  audioInputBuffer[audioInputBufferCount] = analogRead(AUDIO_INPUT_PIN);
  audioInputBufferCount = (audioInputBufferCount + 1) % AUDIO_INPUT_BUFFER_SIZE;
  if (audioInputBufferCount == 0) {
    audioInputBufferFull = 1;
  }
}

/*/
########################################################
  Setup
########################################################
/*/

void setup() {
  // set baud rate
  Serial.begin(115200);
  // wait for Serial to set up
  while (!Serial)
    ;
  Serial.println("Ready");
  delay(1000);

  calculateCosWave();
  calculateSinWaveTable();

  calculateBreadslicerLocations();
  // setup reference tables for aSin[] oscillator array
  for (int i = 0; i < slices; i++) {
    averageAmplitudeOfSlice[i] = 0;
    peakFrequencyOfSlice[i] = int(map(breadslicerSliceLocations[(i % 5)], 1, FFT_WINDOW_SIZE_BY2, frequencyResolution, SAMPLING_FREQ_BY2));    
  }
  for (int i = 0; i < AUDIO_OUTPUT_BUFFER_SIZE; i++) {
    audioOutputBuffer[i] = 0;
    if (i < FFT_WINDOW_SIZE) {
      audioInputBuffer[i] = 0;
      vReal[i] = 0.0;
      vImag[i] = 0.0;
      if (i < FFT_WINDOW_SIZE_BY2) {
        vRealPrev[i] = 0.0;
        FFTData[i] = 0.0;
      }
    }
  }

  // setup pins
  pinMode(AUDIO_INPUT_PIN, INPUT);
  pinMode(AUDIO_OUTPUT_PIN, OUTPUT);

  // get average analog read time over 10,0000 continuous samples
  // unsigned long time = micros();
  // for (int i = 0; i < 10000; i++) {
  //   analogRead(AUDIO_INPUT_PIN);
  // }
  // time = micros() - time;
  // Serial.printf("Average analogRead() time: %d\n", int(round(time / 10000.0)));
  // get average analog write time over 10,000 continous samples
  // time = micros();
  // for (int i = 0; i < 10000; i++) {
  //   analogWrite(AUDIO_OUTPUT_PIN, 0);
  // }
  // time = micros() - time;
  // Serial.printf("Average analogWrite() time: %d\n", int(round(time / 10000.0)));

  // setup interrupt for audio sampling
  My_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(My_timer, &onTimer, true);
  timerAlarmWrite(My_timer, sampleDelayTime, true);
  timerAlarmEnable(My_timer); //Just Enable
}

/*/
########################################################
  Loop
########################################################
/*/

void loop() {
  audioInputBufferFull = 1;
  if (audioInputBufferFull) {
    unsigned long time = micros();
    if (numFFTCount < 3) {
      numFFTCount++;
    }
    // store previously computed FFT results in vRealPrev array, and copy values from audioInputBuffer to vReal array
    setupFFT();
    
    FFT.DCRemoval();
    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
    FFT.Compute(FFT_FORWARD);// Compute FFT
    FFT.ComplexToMagnitude();                      // Compute frequency magnitudes

    // Average the previous and next vReal arrays for a smoother spectrogram
    averageFFTWindows();

    breadslicer(FFTData);
 
    generateAudioForWindow();
    //audioInputBufferFull = 0;
    Serial.println(micros() - time);
  }
}

/*/
########################################################
  Functions related to FFT analysis
########################################################
/*/

/*
  setup arrays for FFT analysis
*/
void setupFFT() {
  for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
    // store the previously generated amplitudes in the vRealPrev array, to smoothen the spectrogram later during processing
    if (i < FFT_WINDOW_SIZE_BY2) {
      vRealPrev[i] = vReal[i];
    }
    // copy values from audio input buffers
    vReal[i] = float(audioInputBuffer[i]);
    // set imaginary values to 0
    vImag[i] = 0.0;
  }
}

/*
  Average the previous and next FFT windows to reduce noise and produce a cleaner spectrogram for signal processing
*/
void averageFFTWindows () {
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i ++) {
    FFTData[i] = (vRealPrev[i] + vReal[i]) / 2.0;
  }
}

/*
  Used to pre-calculate the array locations for the breadslicer to reduce processor load during loop
*/
void calculateBreadslicerLocations() {
  // The curve to follow for binning the amplitudes array: x^(1 / ((x^BREADSLICER_CURVE_EXPONENT) + BREADSLICER_CURVE_OFFSET)) where x is between 0.0 and 1.0
  float step = 1.0 / slices;       // how often to step on the x-axis to determine bin value
  // calculate array locations for the breadslicer
  Serial.println("Calculating array slice locations for breadslicer...");
  //Serial.printf("\tUsing curve: x ^ (1 / ((x ^ %0.3f) + %0.3f)), where X is from 0.0 to 1.0\n", BREADSLICER_CURVE_EXPONENT, BREADSLICER_CURVE_OFFSET);
  // stores the previous slices frequency, only used for printing
  int lastSliceFrequency = 0;
  for (int i = 0; i < slices; i++) {
    float xStep = (i + 1) * step;        // x-axis step (0.0 - 1.0)
    float exponent = 1.0 / (pow(xStep, float(BREADSLICER_CURVE_EXPONENT)) + float(BREADSLICER_CURVE_OFFSET));  // exponent of the curve
    // calculate slice location in array based on the curve to follow for slicing.
    int sliceLocation = round(FFT_WINDOW_SIZE_BY2 * pow(xStep, exponent));
    int sliceFrequency = round(sliceLocation * frequencyResolution);      // Calculates the slices frequency range
    // The breadslicer function uses less-than sliceLocation in its inner loop, which is why we add one.
    if (sliceLocation < FFT_WINDOW_SIZE_BY2) {
      sliceLocation += 1;
    }
    // store location in array that is used by the breadslicer function
    breadslicerSliceLocations[i] = sliceLocation;
    // print the slice locations and it's associated frequency
    //Serial.printf("\t\tsliceLocation %d, Range %dHz to %dHz\n", sliceLocation, lastSliceFrequency, sliceFrequency);
    // store the previous slices frequency
    lastSliceFrequency = sliceFrequency;
  }
  Serial.println("\n");
}

/*
  Splits the amplitude data (*data) into X bands based on somewhat of a parabolic/logarithmic curve, where X is @slices. This is done to detect features of sound which are commonly found at certain frequency ranges.
  For example, 0-200Hz is where bass is common, 200-1200Hz is where voice and instruments are common, 1000-5000Hz higher notes and tones as well as percussion
*/
void breadslicer(float *data) {
  // The maximum amplitude of the array. Set to BREADSLICER_MAX_AVG_BIN to scale all amplitudes to the 0-255 range and to represent volume
  int curMaxAvg = BREADSLICER_MAX_AVG_BIN;

  float topOfSample = 0;  // The frequency of the current amplitude in the amplitudes array

  // lastJ is the last amplitude taken from *data. It is set to 1 to skip the first amplitude at 0Hz
  int lastJ = 1;
  // Calculate the size of each bin and amplitudes per bin
  for (int i = 0; i < slices; i++) {
    // use pre-calculated array locations for amplitude array slicing
    int sliceSize = breadslicerSliceLocations[i];

    // store the array location pre-inner loop
    int newJ = lastJ;

    int ampSliceSum = 0;     // the sum of the current slice
    int ampSliceCount = 0;   // the number of amplitudes in the current slice
    // these values are used to determine the "peak" of the current slice
    int maxSliceAmp = 0;
    int maxSliceAmpFreq = 0;
    // the averageCount, counts how many amplitudes are above the FFT_FLOOR_THRESH, the averageFreq contains the average frequency of these amplitudes
    int ampsAboveThresh = 0;
    int averageFreq = 0;

    // inner loop, finds the average energy of the slice and "peak" frequency
    for (int j = lastJ; j < sliceSize; j++) {
      topOfSample += frequencyResolution;  // calculate the associated frequency
      // store data
      int amp = round(data[j]);
      // if amplitude is above certain threshold and below outlier value, then exclude from signal processing
      if (amp > FFT_FLOOR_THRESH && amp < FFT_OUTLIER) {
        // floor the current amplitude, considered 0 otherwise since its excluded
        //amp -= FFT_FLOOR_THRESH;
        // add to the slice sum, and average frequency, increment amps above threshold counter
        ampSliceSum += amp;
        averageFreq += topOfSample;
        ampsAboveThresh += 1;
        // the peak amplitude of the current slice, this is not exactly the "peak" of the spectogram since slicing the array can cut off peaks. But it is good enough for now.
        if (amp > maxSliceAmp) {
          maxSliceAmp = amp;
          maxSliceAmpFreq = int(round(topOfSample));
        }
      }
      // increment the current slice's amplitude count
      ampSliceCount += 1;
      // save last location in array
      newJ += 1;
      // Debugging stuff
      // Serial.printf("I: %d\tFreq: %.2f\tAmp: %.2f\t", j, topOfSample, data[j]);
      // Serial.println();
    }
    //averageAmplitudeOfSlice[i] = ampSliceSum * weight;
    //if the current slice contains at least two amplitudes, then assign the average, otherwise assign the sum of the slice
    if (ampSliceCount > 1) {
      averageAmplitudeOfSlice[i] = int(round(ampSliceSum / ampSliceCount));
      averageFreq = int(round(averageFreq / ampsAboveThresh));
    } else {
      averageAmplitudeOfSlice[i] = round(ampSliceSum);
    }
    // if there is at least one amplitude that is above threshold in the group, map it's frequency, otherwise frequency for that slice is unchanged
    if (ampsAboveThresh > 0) {
      peakFrequencyOfSlice[i] = maxSliceAmpFreq;
    }
    // ensure that averages aren't higher than the max set average value, and if it is higher, then the max average is replaced by that value, for this iteration
    if (averageAmplitudeOfSlice[i] > curMaxAvg) {
        curMaxAvg = averageAmplitudeOfSlice[i];
    }
    // set the iterator to the next location in the array for the next slice
    lastJ = newJ;
  }
  // put all amplitudes within the 0-255 range, with this "K" value, done in mapFreqAmplitudes()
  amplitudeToRange = float(4095.0 / (curMaxAvg * slices));
}

/*/
########################################################
  Functions related to generating values for the circular audio output buffer
########################################################
/*/

void calculateCosWave() {
  for (int i = 0; i < FFT_WINDOW_SIZE_X2; i++) {
    // calculate values for cosine function that is used to transition between frequencies (0.5 * (1 - cos((2PI / T) * x)), where T = FFT_WINDOW_SIZE_X2
    cos_wave[i] = 0.5 * (1.0 - cos(float(resolution * i)));
  }
}

void calculateSinWaveTable() {
  for (int i = 0; i < SINE_WAVE_TABLE_SIZE; i++) {
    for (int j = 0; j < AUDIO_OUTPUT_BUFFER_SIZE; j++) {
      // calculate values for frequency of (i + 1) within the time domain of "resolution"
      sin_wave_table[i][j] = round(4096.0 * sin(resolution * (i + 1) * j));
    }
  }
}

void generateAudioForWindow() {  
  int offset = 0;
  if (toggleOffsetWave) {
      offset = FFT_WINDOW_SIZE;
  }
  
  for (int i = 0; i < FFT_WINDOW_SIZE_X2; i++) {
    generateAudioCounter = (generateAudioCounter + AUDIO_OUTPUT_BUFFER_SIZE) % AUDIO_OUTPUT_BUFFER_SIZE;
    long int sumOfSines = 0;
    for (int j = 0; j < slices; j++) {
      //sumOfSines += float(averageAmplitudeOfSlice[j] * sin(float(resolution * peakFrequencyOfSlice[j] * (i + offset))));
      // sumOfSines += float(averageAmplitudeOfSlice[j] * sin_wave_table[peakFrequencyOfSlice[j]][i + offset];
      sumOfSines += averageAmplitudeOfSlice[j] * sin_wave_table[j][(i + offset) % FFT_WINDOW_SIZE_X2];
    }
    audioOutputBuffer[generateAudioCounter] += int(round(cos_wave[i] * sumOfSines)) >> 12;
    generateAudioCounter++;
  }
  int lastAudioCounter = generateAudioCounter;
  for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
      audioOutputBuffer[(lastAudioCounter + i) % AUDIO_OUTPUT_BUFFER_SIZE] = 0.0;
  }
  generateAudioCounter -= FFT_WINDOW_SIZE;
  toggleOffsetWave = 1 - toggleOffsetWave;
}

/*
// Online C++ compiler to run C++ program online
#include <iostream>
#include <iomanip>
#include <cmath>
using namespace std;

const float PI = 3.14159;
const int FFT_WINDOW_SIZE = 32;
const int FFT_WINDOW_SIZE_BY2 = FFT_WINDOW_SIZE * 0.5;
const int FFT_WINDOW_SIZE_X2 = FFT_WINDOW_SIZE * 2;
const int AUDIO_OUTPUT_BUFFER_SIZE = FFT_WINDOW_SIZE * 3;

float audioOutputBuffer[AUDIO_OUTPUT_BUFFER_SIZE];

int generateAudioCounter = 0;
int audioOutputBufferIdx = 0;

int toggleOffsetWave = 0;

void generateAudioForWindow() {
  float resolution = float(2.0 * PI / FFT_WINDOW_SIZE_X2);
  
  int offset = 0;
  if (toggleOffsetWave) {
      offset = FFT_WINDOW_SIZE;
  }
  
  
  for (int i = 0; i < FFT_WINDOW_SIZE_X2; i++) {
    generateAudioCounter = (generateAudioCounter + AUDIO_OUTPUT_BUFFER_SIZE) % AUDIO_OUTPUT_BUFFER_SIZE;
    float aCos = 0.5 * (1.0 - cos(float(resolution * i)));
    //cout << generateAudioCounter << '\t' << aCos << endl;
    float sumOfSines = sin(float(resolution * (i + offset)));
    //cout << i << "\t" << float(aCos * sumOfSines) << endl;
    // for (int j = 0; j < slices; j++) {
    //   sumOfSines += averageAmplitudeOfSlice[j] * sin(float(2.0 * PI * peakFrequencyOfSlice[j] * generateAudioCounter));
    // }
    audioOutputBuffer[generateAudioCounter] += float(aCos * sumOfSines);
    generateAudioCounter++;
  }
  int lastAudioCounter = generateAudioCounter;
  for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
      audioOutputBuffer[(lastAudioCounter + i) % AUDIO_OUTPUT_BUFFER_SIZE] = 0.0;
  }
  generateAudioCounter -= FFT_WINDOW_SIZE;
  toggleOffsetWave = 1 - toggleOffsetWave;
}

int main() {
    // Write C++ code here
    for (int k = 0; k < 10; k++) {
        int lastAudioLoc = audioOutputBufferIdx;
        for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
            audioOutputBufferIdx = (audioOutputBufferIdx + 1) % AUDIO_OUTPUT_BUFFER_SIZE;
            
        }
        fftCount++;
        generateAudioForWindow();
        int idxCout = (audioOutputBufferIdx - 1 >= 0) ? audioOutputBufferIdx - 1 : AUDIO_OUTPUT_BUFFER_SIZE - 1;
        cout << "FFTNUM: " << fftCount << "\tAUDIO_BUFFER " << lastAudioLoc << '-' << idxCout << endl;
        for (int j = lastAudioLoc; j <= idxCout; j++) {
        //for (int j = 0; j < AUDIO_OUTPUT_BUFFER_SIZE; j++) {
            cout << j << '\t' << setprecision(2) << audioOutputBuffer[j] << endl;
        }
    }
    

    return 0;
}
*/