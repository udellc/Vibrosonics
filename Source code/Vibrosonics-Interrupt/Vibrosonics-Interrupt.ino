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
#define SAMPLING_FREQ 12000    // The audio sampling and audio output rate, with the ESP32 the fastest we could sample at is just above 16kHz, but
                              // bringing this down to just a tad lower, leaves more room for FFT and other signal processing functions to be done as well
                              // synthesis

#define FFT_FLOOR_THRESH 1000  // amplitude flooring threshold for FFT, to reduce noise

#define AVG_WINDOW 8        // number of windows to average for more consistent frequencies in the low frequency ranges
#define BASS_FREQ 120       // frequencies up to this frequency are considered bass

#define RUNNING_AVG_ARR_SIZE 20

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

const float frequencyWidth = float(FFT_WINDOW_SIZE) / float(SAMPLING_FREQ);

float vRealPrev[FFT_WINDOW_SIZE_BY2]; // stores the previous amplitudes calculated by FFT, for averaging windows

float FFTData[FFT_WINDOW_SIZE_BY2];   // stores the data the signal processing functions will use

float FFTDataPrev[FFT_WINDOW_SIZE_BY2]; // stores the previous data used for analysis to find frequency of max amplitude change between consecutive windows
const int FREQ_MAX_AMP_DELTA_MIN = 500;   // the threshold for a change in amplitude to be considered significant by the frequencyMaxAmplitudeDelta() function, basically the sensitivity
const float FREQ_MAX_AMP_DELTA_K = 2.0;   // multiplier for amplitude of most change
int maxAmpChangeDetected = 0;
int maxAmpChange = 0;
int maxAmpChangeIdx = 0;

const int freqsToAverage = ceil(float(BASS_FREQ) / frequencyResolution);
int averageWindowCount = 0;
float FFTWindows[int(AVG_WINDOW)][freqsToAverage];

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

// sine wave for storing pre-generated values of a sine wave at SAMPLING_FREQ sample rate
float sin_wave[SAMPLING_FREQ];

// stores the position of sin_wave
int sin_wave_idx = 0;

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
      FFTDataPrev[i] = 0.0;
    }
    if (i < FFT_WINDOW_SIZE_BY2 >> 1) {
      majorPeaksFreq[i] = 0;
      majorPeaksAmplitude[i] = 0;
    }
  }

  // setup pins
  pinMode(AUDIO_OUTPUT_PIN, OUTPUT);
  pinMode(AUDIO_INPUT_PIN, INPUT);

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

  // setup timer interrupt for audio sampling
  My_timer = timerBegin(0, 80, true);                 // set clock prescaler 80MHz / 80 = 1MHz
  timerAttachInterrupt(My_timer, &onTimer, true);     // attach interrupt function
  timerAlarmWrite(My_timer, sampleDelayTime, true);   // trigger interrupt every @sampleDelayTime microseconds
  timerAlarmEnable(My_timer);     //Just Enable
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

    // reset flag
    audioInputBufferFull = 0;
    
    FFT.DCRemoval();                                  // DC Removal to reduce noise
    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
    FFT.Compute(FFT_FORWARD);                         // Compute FFT
    FFT.ComplexToMagnitude();                         // Compute frequency magnitudes

    // Average the previous and next vReal arrays for a smoother spectrogram
    averageFFTWindows();

    // for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
    //   Serial.printf("%d, %.2f\n", i, FFTData[i]);
    // }

    frequencyMaxAmplitudeDelta(FFTData, FFTDataPrev, FFT_WINDOW_SIZE_BY2);

    // if (maxAmpChangeDetected) {
    //   Serial.printf("mi %d mx %d f %d\n", 0, SAMPLING_FREQ_BY2, int(round(maxAmpChangeIdx * frequencyResolution)), maxAmpChange);
    // }

    int totalPeaksFound = findMajorPeaks(FFTData, 4);

    int maxAmp = 0;
    int maxAmpIdx = 0;
    int totalEnergy = 0;
    // iterate through peaks found by the major peaks function
    for (int i = 0; i < totalPeaksFound; i++) {
      int amplitude = majorPeaksAmplitude[i];
      // find max amplitude
      if (amplitude > maxAmp) {
        maxAmp = amplitude;
        maxAmpIdx = i;
      }
      // if frequencyOfMaxAmplitudeChange() found a amplitude change above threshold and it exists within a range of a major peak, then weigh this amplitude
      if (maxAmpChangeDetected && ((majorPeaksFreq[i] > (maxAmpChangeIdx - 1) * frequencyResolution) && (majorPeaksFreq[i] < (maxAmpChangeIdx + 1) * frequencyResolution))) {
        amplitude = int(amplitude * 15.0);
        majorPeaksAmplitude[i] = amplitude;

      }
      // sum the energy of these peaks
      totalEnergy += amplitude;

      //Serial.printf("(F: %04d, A: %04d) ", majorPeaksFreq[i], amplitude);
    }
    // weight of the max amplitude
    float maxAmpWeight = maxAmp / float(totalEnergy);
    // if the weight of max amplitude is larger than a certain threshold, assume that a single frequency is being detected to decrease noise in the output
    int singleFrequency = maxAmpWeight > 0.9 ? 1 : 0;
    //Serial.println();
    // value to map amplitudes between 0.0 and 1.0 range, the minTotalEnergy will be used to divide unless totalEnergy exceeds this value
    float divideBy = 1.0 / float(totalEnergy > 3000 ? (totalEnergy) : 3000);

    // normalizing and multiplying to ensure that the sum of amplitudes is less than or equal to 255
    for (int i = 0; i < totalPeaksFound; i++) {
      int amplitude = majorPeaksAmplitude[i];
      // map amplitude between 0 and 127
      int mappedAmplitude = round(amplitude * divideBy * 127.0);
      // if assuming single frequency is detected based on the weight of the amplitude,
      if (singleFrequency) {
        // assign other amplitudes to 0 to reduce noise
        majorPeaksAmplitude[i] = i == maxAmpIdx ? mappedAmplitude : 0;
      } else {
        // otherwise assign normalized amplitudes
        majorPeaksAmplitude[i] = mappedAmplitude;
      }

      // interpolate around the peak and convert from index to frequency
      int frequency = interpolateAroundPeak(FFTData, majorPeaksFreq[i]);

      if (frequency < BASS_FREQ) { majorPeaksFreq[i] = frequency; }
      //else if (frequency < 1100) { majorPeaksFreq[i] = round(frequency * 0.125) + 30; }
      else { majorPeaksFreq[i] = round(frequency * 0.03) + 30; }

      // if (majorPeaksAmplitude[i] > 0) {
      //   Serial.printf("(F: %04d, A: %04d) ", majorPeaksFreq[i], majorPeaksAmplitude[i]);
      // }
    }
    //Serial.println();

    // map the amplitudes and frequencies calculated by the signal processing functions
    //mapAmplitudeFrequency();

    // generate audio for the next audio window
    generateAudioForWindow(majorPeaksFreq, majorPeaksAmplitude, totalPeaksFound);

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
    // store the previously generated amplitudes in the vRealPrev array, to smoothen the spectrogram later during processing
    if (i < FFT_WINDOW_SIZE_BY2) { 
      vRealPrev[i] = vReal[i]; 
    }
    // copy values from audio input buffer
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
    if (i < freqsToAverage) {
      // store low frequencies in FFTWindows array to reduce aliasing
      FFTWindows[averageWindowCount][i] = vReal[i];
      float amplitudeSum = 0.0;
      for (int j = 0; j < AVG_WINDOW; j++) {
        amplitudeSum += FFTWindows[j][i];
      }
      FFTData[i] = amplitudeSum / float(AVG_WINDOW);
      //Serial.printf("idx: %d, vReal: %.2f, data: %.2f\n", i, vReal[i], FFTData[i]);
    } else {
      FFTData[i] = (vRealPrev[i] + vReal[i]) * 0.5;
      //FFTData[i] = vReal[i];
    }
  }
  averageWindowCount = (averageWindowCount + 1) % int(AVG_WINDOW);
}


void frequencyMaxAmplitudeDelta(float *data, float *prevData, int arraySize) {
  // restore global varialbes
  maxAmpChangeDetected = 0;
  maxAmpChange = 0;
  // iterate through data* and prevData* to find the amplitude with most change
  for (int i = 0; i < arraySize; i++) {
    // store the change of between this amplitude and previous amplitude
    int currAmpChange = abs(int(data[i]) - int(prevData[i]));
    // find the most dominant amplitude change and store in maxAmpChangeIdx, store the magnitude of the change in maxAmpChange
    if (currAmpChange > maxAmpChange && currAmpChange > FREQ_MAX_AMP_DELTA_MIN) {
      maxAmpChangeDetected = 1;
      maxAmpChange = currAmpChange;
      maxAmpChangeIdx = i;
    }
    // assign data to previous data
    prevData[i] = data[i];
  }
}


int findMajorPeaks(float* data, int maxNumPeaks) {
  // restore output arrays
  for (int i = 0; i < FFT_WINDOW_SIZE_BY2 >> 1; i++) {
    majorPeaksAmplitude[i] = 0;
    majorPeaksFreq[i] = 0;
  }
  int peaksFound = 0;
  int maxPeak = 0;
  int maxPeakIdx = 0;
  // iterate through data to find peaks
  for (int f = 1; f < FFT_WINDOW_SIZE_BY2 - 1; f++) {
    if (data[f] > float(FFT_FLOOR_THRESH)) {
      // determines if data[f] is a peak by comparing with previous and next location, otherwise continue
      if ((data[f - 1] < data[f]) && (data[f] > data[f + 1])) {
        if (data[f] > maxPeak) {
          maxPeak = data[f];
          maxPeakIdx = f;
        }
        // summing around the index of peak to get a better representation of the energy associated with the peak
        int peakSum = round((data[f - 1] + data[f] + data[f + 1]) * frequencyWidth);
        majorPeaksAmplitude[peaksFound] = peakSum;
        // return interpolated frequency
        majorPeaksFreq[peaksFound] = f;

        peaksFound += 1;
      }
    }
  }

  // if needed, remove a certain number of the minumum peaks to contain output to @maxNumberOfPeaks
  int numPeaksToRemove = peaksFound - maxNumPeaks;
  for (int j = 0; j < numPeaksToRemove; j++) {
    // store minimum as the maximum peak
    int minimumPeak = maxPeak;
    int minimumPeakIdx = maxPeakIdx;
    // find the minimum peak and replace with zero
    for (int i = 1; i < peaksFound; i++) {
      int thisPeakAmplitude = majorPeaksAmplitude[i];
      if (thisPeakAmplitude > 0 && thisPeakAmplitude < minimumPeak) {
        minimumPeak = thisPeakAmplitude;
        minimumPeakIdx = i;
      }
    }
    majorPeaksAmplitude[minimumPeakIdx] = 0;
  }
  return peaksFound;
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

void mapAmplitudeFrequency() {
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

void generateAudioForWindow(int *sin_waves_freq, int *sin_wave_amp, int num_sin_waves) {
  for (int i = 0; i < FFT_WINDOW_SIZE_X2; i++) {
    // variable store the value of the sum of sine waves at this particular moment
    float sumOfSines = 0.0;
    // sum together the sine waves
    for (int s = 0; s < num_sin_waves; s++) {
      // calculate the sine wave index of the sine wave corresponding to the frequency, offsetting by 'random' polynomial to reduce sharp transitions
      if (sin_wave_amp[s] > 0) {
        long sin_wave_freq_idx = long(sin_wave_idx) * sin_waves_freq[s];
        int sin_wave_position = sin_wave_freq_idx % int(SAMPLING_FREQ);
        sumOfSines += sin_wave_amp[s] * sin_wave[sin_wave_position];
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
  sin_wave_idx = ((sin_wave_idx - FFT_WINDOW_SIZE) + int(SAMPLING_FREQ)) % int(SAMPLING_FREQ);
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

