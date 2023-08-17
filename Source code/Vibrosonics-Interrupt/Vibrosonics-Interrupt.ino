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

#define AUDIO_INPUT_PIN A2
#define AUDIO_OUTPUT_PIN A0

#define FFT_WINDOW_SIZE 256   // Number of FFT windows, this value must be a power of 2
#define SAMPLING_FREQ 10000    // The audio sampling and audio output rate, with the ESP32 the fastest we could sample at is just above 16kHz, but
                              // bringing this down to just a tad lower, leaves more room for FFT and other signal processing functions to be done as well
                              // synthesis

#define FFT_FLOOR_THRESH 2000  // amplitude flooring threshold for FFT, to reduce noise
#define FFT_OUTLIER 30000     // Outlier for unusually high amplitudes in FFT

#define BASS_FREQ 250       // frequencies up to this frequency are considered bass

#define BREADSLICER_NUM_SLICES 5 // The number of slices the breadslicer does
#define BREADSLICER_CURVE_EXPONENT 3.0  // The exponent for the used for the breadslicer curve
#define BREADSLICER_CURVE_OFFSET 0.59  // The curve offset for the breadslicer to follow when slicing the amplitude array
#define BREADSLICER_MAX_AVG_BIN 2000 // The minimum max that is used for scaling the amplitudes to the 0-255 range, and helps represent the volume

#define RUNNING_AVG_ARR_SIZE 2

#define SIN_WAVE_TABLE_SIZE 160  // How many frequencies to pre-generate (i.e from 1 to SIN_WAVE_TABLE_SIZE Hz). This is used to speed up calculations performed by generateAudioForWindow()

#define AUDIO_INPUT_BUFFER_SIZE int(FFT_WINDOW_SIZE)
#define AUDIO_OUTPUT_BUFFER_SIZE int(FFT_WINDOW_SIZE) * 3

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

const int frequencyResolution = float(SAMPLING_FREQ) / float(FFT_WINDOW_SIZE);  // the frequency resolution of FFT with the current window size

float vRealPrev[FFT_WINDOW_SIZE_BY2]; // stores the previous amplitudes calculated by FFT, for averaging windows

float FFTData[FFT_WINDOW_SIZE_BY2];   // stores the data the signal processing functions will use

const int slices = BREADSLICER_NUM_SLICES;

float amplitudeToRange = (128.0/BREADSLICER_MAX_AVG_BIN) / slices;   // the "K" value used to put the average amplitudes calculated by breadslicer into the 0-255 range. This value alters but doesn't go below BREADSLICER_MAX_AVG_BIN to give a sense of the volume

const int breadslicerSliceLocationsStatic[slices] {250, 500, 1200, 2800, 5000};
int breadslicerSliceLocations[slices];
float breadslicerSliceWeights[slices] {1.0, 1.0, 1.0, 1.0, 1.0};

int averageAmplitudeOfSlice[slices];          // the array used to store the average amplitudes calculated by the breadslicer
int peakFrequencyOfSlice[slices];             // the array containing the peak frequency of the slice

int runningAverageArray[RUNNING_AVG_ARR_SIZE];

int majorPeakAmplitude = 0;

float majorPeakAmp = 0.0;

int majorPeakFrequency = 0;

int majorPeaksFreq[FFT_WINDOW_SIZE_BY2 >> 1];
int majorPeaksAmplitude[FFT_WINDOW_SIZE_BY2 >> 1];

/*/
########################################################
  Variables used for generating an audio output
########################################################
/*/

const int FFT_WINDOW_SIZE_X2 = FFT_WINDOW_SIZE * 2;

// a cosine wave for modulating sine waves
float cos_wave[FFT_WINDOW_SIZE_X2];

// sine wave for storing pre-generated values of sine waves at SAMPLING_FREQ sample rate
float sin_wave[SAMPLING_FREQ];

// stores the position of sin_wave
int sin_wave_idx = 0;

int sinWaveAmplitude[BREADSLICER_NUM_SLICES];
int sinWaveFrequency[BREADSLICER_NUM_SLICES];

/*/
########################################################
  Variables used for interrupt and function to call when interrupt is triggered
########################################################
/*/

// used to store recorded samples
volatile int audioInputBuffer[AUDIO_INPUT_BUFFER_SIZE];
volatile int audioInputBufferIdx = 0;
volatile int audioInputBufferFull = 0;

// rolling buffer for outputting synthesized signal
volatile int audioOutputBufferVolatile[FFT_WINDOW_SIZE_X2];
volatile int audioOutputBufferIdx = 0;

// scratchpad array used for synthesis
float audioOutputBuffer[AUDIO_OUTPUT_BUFFER_SIZE];
// stores the current index of the scratch pad audio output buffer
int generateAudioIdx = 0;
// used for copying final synthesized values from scratchpad audio output buffer to volatile audio output buffer
int rollingAudioOutputBufferIdx = 0;

// used to delay the audio output buffer by 2 FFT sampling periods so that audio can be generated on time, this results in a delay between input and output of 2 * FFT_WINDOW_SIZE / SAMPLING_FREQ
volatile int numFFTCount = 0;

hw_timer_t *My_timer = NULL;

void IRAM_ATTR onTimer(){
  outputSample();
  recordSample();
}

/*/
########################################################
  Setup
########################################################
/*/

void setup() {
  // set baud rate
  Serial.begin(115200);
  analogReadResolution(12);
  // wait for Serial to set up
  while (!Serial)
    ;
  delay(3000);
  Serial.println("Ready");

  calculateCosWave();
  calculateSinWave();

  calculateBreadslicerLocations();
  // setup reference tables for aSin[] oscillator array
  for (int i = 0; i < slices; i++) {
    sinWaveAmplitude[i] = 0;
    sinWaveFrequency[i] = 0;
    averageAmplitudeOfSlice[i] = 0;
    peakFrequencyOfSlice[i] = int(map(breadslicerSliceLocations[(i % 5)], 1, FFT_WINDOW_SIZE_BY2, frequencyResolution, SAMPLING_FREQ_BY2));
  }
  for (int i = 0; i < AUDIO_OUTPUT_BUFFER_SIZE; i++) {
    audioOutputBuffer[i] = 0.0;
    if (i < FFT_WINDOW_SIZE_X2) {
      audioOutputBufferVolatile[i] = 0;
    }
    if (i < FFT_WINDOW_SIZE) {
      audioInputBuffer[i] = 0;
      vReal[i] = 0.0;
      vImag[i] = 0.0;
    }
    if (i < FFT_WINDOW_SIZE_BY2) {
      vRealPrev[i] = 0.0;
      FFTData[i] = 0.0;
    }
    if (i < FFT_WINDOW_SIZE_BY2 >> 1) {
      majorPeaksFreq[i] = 0;
      majorPeaksAmplitude[i] = 0;
    }
  }
  // generate random seed with noise from floating pin

  // setup pins
  pinMode(AUDIO_OUTPUT_PIN, OUTPUT);
  pinMode(AUDIO_INPUT_PIN, INPUT);

  //randomSeed(analogRead(AUDIO_INPUT_PIN));

  Serial.println("Setup complete");

  // while (1) {
  //   if (Serial.available())
  //   {
  //     while (Serial.available())
  //     {
  //       Serial.read();
  //     }
  //     break;
  //   }
  // }

  // setup interrupt for audio sampling
  My_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(My_timer, &onTimer, true);
  timerAlarmWrite(My_timer, sampleDelayTime, true);
  timerAlarmEnable(My_timer); //Just Enable
  timerStart(My_timer);
}

/*/
########################################################
  Loop
########################################################
/*/

void loop() {
  if (audioInputBufferFull == 1) {
    //timerAlarmDisable(My_timer);

    //unsigned long time = micros();
    // store previously computed FFT results in vRealPrev array, and copy values from audioInputBuffer to vReal array
    setupFFT();

    audioInputBufferFull = 0;
    
    FFT.DCRemoval();                                  // DC Removal to reduce noise
    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
    FFT.Compute(FFT_FORWARD);                         // Compute FFT
    FFT.ComplexToMagnitude();                         // Compute frequency magnitudes

    // Average the previous and next vReal arrays for a smoother spectrogram
    averageFFTWindows();

    // for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    //   Serial.printf("%d, %d, %.2f\n", 0, 30000, FFTData[i]);
    // }

    findMajorPeaks(FFTData, 16);

    int numAmplitudes = 0;
    int totalEnergy = 0;
    for (int i = 0; i < FFT_WINDOW_SIZE_BY2 >> 1; i++) {
      int amplitude = majorPeaksAmplitude[i];
      if (amplitude > 0) {
        totalEnergy += amplitude;
        
        numAmplitudes += 1;
        //Serial.printf("(F: %04d, A: %04d) ", majorPeaksFreq[i], amplitude);
      } else { break; }
    }
    //Serial.println();
    // int divideBy;
    // if (totalEnergy > 5000) {
    //   divideBy = totalEnergy;
    // }
    int divideBy = totalEnergy > 5000 ? (totalEnergy + 500) : 5000;

    // normalizing and multiplying to ensure that the sum of amplitudes is less than or equal to 255
    for (int i = 0; i < FFT_WINDOW_SIZE_BY2 >> 1; i++) {
      int amplitude = majorPeaksAmplitude[i];
      if (amplitude > 0) {
        majorPeaksAmplitude[i] = float(amplitude) / divideBy * 255.0;
        // if (majorPeaksFreq[i] > 80) {
        //   majorPeaksFreq[i] = map(majorPeaksFreq[i], 80, int(SAMPLING_FREQ_BY2), 60, 250);
        // }
        //Serial.printf("(F: %04d, A: %04d) ", majorPeaksFreq[i], majorPeaksAmplitude[i]);
      } else { break; }
    }
    //Serial.println();

    // int majorPeakF = getRunningAverage(findMajorPeak(FFTData), runningAverageArray, RUNNING_AVG_ARR_SIZE);
    
    // if (majorPeakF >= SIN_WAVE_TABLE_SIZE) {
    //   majorPeakF = map(majorPeakF, 0, SAMPLING_FREQ_BY2, 30, SIN_WAVE_TABLE_SIZE);
    // }

    // if (majorPeakAmplitude > 250) {
    //   majorPeakAmp = float(map(majorPeakAmplitude, 100, majorPeakAmplitude > 500 ? (majorPeakAmplitude + 100) : 1000, 0, 127));
    //   majorPeakFrequency = majorPeakF;
    // } else { majorPeakAmp = 0.0; }
    //Serial.printf("%03d, %03d\n", majorPeakFrequency, majorPeakAmplitude);

    // use the breadslicer to split the spectogram into bands
    //breadslicer(FFTData);

    // map the amplitudes and frequencies calculated by the signal processing functions
    //mapAmplitudeFrequency();

    // generate audio for the next audio window
    generateAudioForWindow(numAmplitudes);

    // int curr_pos = (rollingAudioOutputBufferIdx - int(FFT_WINDOW_SIZE) + int(FFT_WINDOW_SIZE_X2)) % int(FFT_WINDOW_SIZE_X2);
    // for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
    //   Serial.printf("%d %d %d\n", 0, 255, audioOutputBufferVolatile[curr_pos + i]);
    // }

    // while (1) {
    //   if (Serial.available())
    //   {
    //     while (Serial.available())
    //     {
    //       Serial.read();
    //     }
    //     break;
    //   }
    // }
    //timerAlarmEnable(My_timer);
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
    // copy values from audio input buffers
    vReal[i] = float(audioInputBuffer[i]);
    // set imaginary values to 0
    vImag[i] = 0.0;
    // store the previously generated amplitudes in the vRealPrev array, to smoothen the spectrogram later during processing
    if (i < FFT_WINDOW_SIZE_BY2) { vRealPrev[i] = vReal[i]; }
  }
}

/*
  Average the previous and next FFT windows to reduce noise and produce a cleaner spectrogram for signal processing
*/
void averageFFTWindows () {
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i ++) {
    //FFTData[i] = (vRealPrev[i] + vReal[i]) * 0.5;
    FFTData[i] = vReal[i];
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

void findMajorPeaks(float* data, int maxNumPeaks) {
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2 >> 1; i++) {
    majorPeaksAmplitude[i] = 0;
    majorPeaksFreq[i] = 0;
  }
  int peaksFound = 0;
  // iterate through data to find peaks
  for (int f = 1; f < FFT_WINDOW_SIZE_BY2 - 1; f++) {
    if (data[f] > float(FFT_FLOOR_THRESH)) {
      // determines if data[f] is a peak by comparing with previous and next location, otherwise continue
      if ((data[f - 1] < data[f]) && (data[f] > data[f + 1])) {
        // summing around the index of peak to get a better representation of the energy associated with the peak
        majorPeaksAmplitude[peaksFound] = round((data[f - 1] + data[f] + data[f + 1]) * (1.0 / frequencyResolution));
        // return interpolated frequency
        majorPeaksFreq[peaksFound] = interpolateAroundPeak(data, f);

        peaksFound += 1;
      }
    }
  }

  int numPeaksToRemove = peaksFound - maxNumPeaks;
  if (numPeaksToRemove > 0) {
    for (int j = 0; j < numPeaksToRemove; j++) {
      int minimumPeak = majorPeaksAmplitude[0];
      int minimumPeakIdx = 0;
      for (int i = 1; i < peaksFound; i++) {
        int thisPeakAmplitude = majorPeaksAmplitude[i];
        if ((thisPeakAmplitude > 0) && (thisPeakAmplitude < minimumPeak)) {
          minimumPeak = thisPeakAmplitude;
          minimumPeakIdx = 0;
        }
      }
      majorPeaksAmplitude[minimumPeakIdx] = 0;
    }
  }
  //Serial.print(peaksFound);
}

int getRunningAverage(int data, int *outputArray, int arraySize) {
  int arraySum = 0;
  int replaceIdx = 0;
  for (int i = 0; i < arraySize; i++) {
    if (outputArray[i] == -1) {
      arraySum += data;
      outputArray[i] = data;
      replaceIdx = (i + 1) % arraySize;
    } else {
      arraySum += outputArray[i];
    }
  }
  outputArray[replaceIdx] = -1;

  return round(arraySum / arraySize);
}

int interpolateAroundPeak(float *data, int indexOfPeak) {
  float prePeak = data[indexOfPeak - 1];
  float atPeak = data[indexOfPeak];
  float postPeak = data[indexOfPeak + 1];
  // summing around the index of maximum amplitude to normalize magnitudeOfChange
  float peakSum = prePeak + atPeak + postPeak;
  // interpolating the direction and magnitude of change, and normalizing from -1.0 to 1.0
  float magnitudeOfChange = ((atPeak + postPeak) - (atPeak + prePeak)) / peakSum;
  
  // return interpolated frequency
  return round(float((indexOfPeak + magnitudeOfChange) * frequencyResolution));
}

/* my version of the FFT MajorPeak function, uses the same approach to determine the maximum amplitude, but a bit of a different approach to interpolation
   - returns interpolated frequency of the peak
*/
int findMajorPeak(float *data) {
  float maxAmp = 0;
  int idxOfMaxAmp = 0;
  // iterate through data to find maximum peak
  for (int f = 1; f < FFT_WINDOW_SIZE_BY2; f++) {
    // determines if data[f] is a peak by comparing with previous and next location, otherwise continue
    if ((data[f - 1] < data[f]) && (data[f] > data[f + 1])) {
      // compare with previous max amplitude at peak
      if (data[f] > maxAmp) {
        maxAmp = data[f];
        idxOfMaxAmp = f;
      }
    }
  }
  majorPeakAmplitude = round((data[idxOfMaxAmp - 1] + data[idxOfMaxAmp] + data[idxOfMaxAmp + 1]) * (1.0 / frequencyResolution));
  // return interpolated frequency
  return interpolateAroundPeak(data, idxOfMaxAmp);
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

    long ampSliceSum = 0;     // the sum of the current slice
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
    // if there is at least one amplitude that is above threshold in the group, store it's peak frequency, otherwise frequency for that slice is unchanged
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
  amplitudeToRange = float(128.0 / curMaxAvg / slices);
}

void mapAmplitudeFrequency() {
  for (int i = 0; i < BREADSLICER_NUM_SLICES; i++) {
    sinWaveAmplitude[i] = averageAmplitudeOfSlice[i] * amplitudeToRange;
    if (peakFrequencyOfSlice[i] < BASS_FREQ) {
      sinWaveFrequency[i] = round(peakFrequencyOfSlice[i] * 0.26);
    } else {
      sinWaveFrequency[i] = map(peakFrequencyOfSlice[i], int(BASS_FREQ), int(SAMPLING_FREQ_BY2), 10, 199);
    }
  }
}

/*/
########################################################
  Functions related to generating values for the circular audio output buffer
########################################################
/*/

void calculateCosWave() {
  float resolution = float(2.0 * PI / FFT_WINDOW_SIZE_X2);
  // calculate values for cosine function that is used to transition between frequencies (0.5 * (1 - cos((2PI / T) * x)), where T = FFT_WINDOW_SIZE_X2
  for (int i = 0; i < FFT_WINDOW_SIZE_X2; i++) {
    cos_wave[i] = 0.5 * (1.0 - cos(float(resolution * i)));
  }
}

void calculateSinWave() {
  float resolution = float(2.0 * PI / SAMPLING_FREQ);
  // calculate values for 1Hz sine wave at SAMPLING_FREQ sample rate
  for (int x = 0; x < SAMPLING_FREQ; x++) {
    sin_wave[x] = sin(float(resolution * x));
  }
}

void generateAudioForWindow(int numSineWaves) {
  for (int i = 0; i < FFT_WINDOW_SIZE_X2; i++) {
    // variable store the value of the sum of sine waves at this particular moment
    float sumOfSines = 0.0;
    // sum together the sine waves
    for (int s = 0; s < numSineWaves + 1; s++) {
      // calculate the sine wave index of the sine wave corresponding to the frequency, offsetting by 'random' polynomial to reduce sharp transitions
      if (majorPeaksAmplitude[s] > 0) {
        long sin_wave_freq_idx = long(sin_wave_idx) * majorPeaksFreq[s];
        int sin_wave_position = sin_wave_freq_idx % int(SAMPLING_FREQ);
        sumOfSines += majorPeaksAmplitude[s] * sin_wave[sin_wave_position];
      }
    }
    // multiply cos wave and sum of sines at this moment in time
    float synthesized_value = cos_wave[i] * sumOfSines;
    // add to the existing values in scratch pad audio output buffer
    audioOutputBuffer[generateAudioIdx] += synthesized_value;

    // copy final, synthesized values to volatile audio output buffer
    if (i < FFT_WINDOW_SIZE) {
      audioOutputBufferVolatile[rollingAudioOutputBufferIdx++] = round(audioOutputBuffer[generateAudioIdx] + 128.0);
      if (rollingAudioOutputBufferIdx == FFT_WINDOW_SIZE_X2) { rollingAudioOutputBufferIdx = 0; }
    }
    // increment generate audio index
    generateAudioIdx += 1;
    if (generateAudioIdx == AUDIO_OUTPUT_BUFFER_SIZE) { generateAudioIdx = 0; }
    // increment sine wave index
    sin_wave_idx += 1;
    if (sin_wave_idx == SAMPLING_FREQ) { sin_wave_idx = 0; }
  }

  // reset the next window to synthesize new signal
  int generateAudioIdxCpy = generateAudioIdx;
  for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
    audioOutputBuffer[generateAudioIdxCpy++] = 0.0;
    if (generateAudioIdxCpy == AUDIO_OUTPUT_BUFFER_SIZE) { generateAudioIdxCpy = 0; }
  }

  // determine the next position in the scratch pad audio output buffer to counter phase cosine wave
  generateAudioIdx = ((generateAudioIdx - int(FFT_WINDOW_SIZE)) + int(AUDIO_OUTPUT_BUFFER_SIZE)) % int(AUDIO_OUTPUT_BUFFER_SIZE);
  //sin_wave_idx = ((sin_wave_idx - FFT_WINDOW_SIZE) + int(SAMPLING_FREQ)) % int(SAMPLING_FREQ);
}

void outputSample() {
  if (numFFTCount > 1) {
    dacWrite(AUDIO_OUTPUT_PIN, audioOutputBufferVolatile[audioOutputBufferIdx++]);
    if (audioOutputBufferIdx == FFT_WINDOW_SIZE_X2) {
      audioOutputBufferIdx = 0;
    }
  }
}

void recordSample() {
  audioInputBuffer[audioInputBufferIdx++] = adc1_get_raw(ADC1_CHANNEL_6);
  if (audioInputBufferIdx == AUDIO_INPUT_BUFFER_SIZE) {
    if (numFFTCount < 2) { numFFTCount++; }
    audioInputBufferFull = 1;
    audioInputBufferIdx = 0;
  }
}

void outputSampleTest() {
  int sin_wave_pos = (audioOutputBufferIdx * majorPeakFrequency) % int(SAMPLING_FREQ);
  int write_sample = round(127.0 * sin_wave[sin_wave_pos] + 128.0);
  dacWrite(AUDIO_OUTPUT_PIN, write_sample);
  // audioOutputBufferIdx = (audioOutputBufferIdx + 1) % sin_wave_table_frequency_period[majorPeakFrequency];
  audioOutputBufferIdx = (audioOutputBufferIdx + 1) % int(SAMPLING_FREQ);
}

