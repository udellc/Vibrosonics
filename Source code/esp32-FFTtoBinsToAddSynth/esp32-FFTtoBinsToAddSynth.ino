#include <arduinoFFT.h>
#include <Arduino.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>
#include <tables/cos2048_int8.h>
#include <Smooth.h>
#include <EventDelay.h>
#include <string.h>

/*/
########################################################
    Directives
########################################################
/*/

#define AUDIO_INPUT_PIN A2          // audio in pin

#define FFT_FLOOR_THRESH 150        // floor threshold for FFT, may be later changed to use the minimum from FFT

#define FREQ_AVG_WIN 6              // Number of FFT windows worth of frequency date to calculate, record, and average when finding the change in dominant frequency of the signal.
#define AMP_AVG_TIME 0.25           // Amount of time to average the signal amplitude across to determine its average amplitude.
#define FREQ_SIG_THRESH 10          // Minimum value required for that the major peak of a signal must meet in order for it to be considered the new dominant frequency of the signal.
#define FREQ_DELTA_ERROR_MARGIN 32  // Minimum difference between dominant frequencies to be considered a significant change.

#define CONTROL_RATE 1024

/*/
########################################################
    UPDATE RATE
########################################################
/*/
int SAMPLE_RATE = int(CONTROL_RATE) >> 6;
int FADE_RATE = CONTROL_RATE / SAMPLE_RATE;

/*/
########################################################
    FFT Setup
########################################################
/*/

/*
These values can be changed in order to evaluate the functions
*/
const uint16_t FFT_WIN_SIZE = 256;  //This value MUST ALWAYS be a power of 2
const float SAMPLE_FREQ = 10000;    //The sampling frequency

/*
These are the input and output vectors
  - vReal is used for input and receives computed results from FFT
  - vImag is used to store imaginary values for computation
*/
float vReal[FFT_WIN_SIZE];
float vImag[FFT_WIN_SIZE];

arduinoFFT FFT = arduinoFFT(vReal, vImag, FFT_WIN_SIZE, SAMPLE_FREQ); // Object for performing FFT's

/*/
########################################################
    Stuff relevant to splitting fft amplitudes into bins
########################################################
/*/

/*
These are used for splitting the amplitudes from FFT into bins,
  - bins integer accounts for how many bins the samples are split into
  - float curve accounts for the curve used when splitting the bins
  - outlier is used to remove unusually high amplitudes from FFT analysis
*/
int bins = 5;
float curve = 0.65;

int maxAvgAmp = 0;
const float OUTLIER = 250000.0;

float sFreqBy2 = SAMPLE_FREQ / 2;  // determine what frequency each amplitude represents. NOTE: unsure if it is accurate
float sFreqBySamples = float(sFreqBy2 / (FFT_WIN_SIZE >> 1));

/* running max and raw input array
  - runningMaxArr stores the last RUN_MAX_ARR_SIZE maximum amplitudes
  - rawInputArr stores the last RAW_INPUT_ARR_SIZE raw audio input for calculating standard deviation
*/
int sampleMax = 0;
int sampleRangeThreshold = 0.0;
int sampleRange = 0.0;

int sampleRangeMax = 0;
int sampleRangeMin = 4096;

unsigned long microseconds;

/*/
########################################################
    Stuff relevant to all of the signal processing functions (calculateCentroid, calculateFrequencyDelta, and calculateAvgAmplitude)
########################################################
/*/
// Number of microseconds to wait between recording samples.
const int sampleDelayTime = round(1000000 / SAMPLE_FREQ);

// Number of samples that should be averaged to calculate the average amplitude of the signal.
const int ampAvgSampCount = round(AMP_AVG_TIME * SAMPLE_FREQ);

// Number of samples required to calculate the all of the FFT windows that are averaged when finding changes in the dominant frequency of the signal.
const int freqAvgSampCount = FREQ_AVG_WIN * FFT_WIN_SIZE;

// Size of the sample buffer array. This expression is equivalent to max(ampAvgSampCount, freqAvgSampCount), but I couldn't use max() because of a weird error.
const int sampleBuffSize = ampAvgSampCount * (ampAvgSampCount > freqAvgSampCount) + freqAvgSampCount * (ampAvgSampCount <= freqAvgSampCount);

// Array that samples are first recorded into. Its size is equal to the minimum size required for performing all of the signal processing.
float samples[sampleBuffSize];

// The number of audio samples taken, used to synchronously implement Nanolux code with the current FFT implementation
int sampleBuffCount = 0;

/*/
########################################################
    Stuff relevant to calculateFreqDelta
########################################################
/*/
// Array used for storing frequency magnitudes from multiple FFT's of the signal. The first index indicates the specific FFT window (and can be thought of as the time at
// which each FFT window was calculated), and the second index indicates the sample index within each frequency window (and can be though of as frequency).
float freqs[FREQ_AVG_WIN][FFT_WIN_SIZE / 2];

// Previously recorded dominant frequency of the signal, calculated by calculateFreqDelta.
float previousDomFreq = 0;

// Current dominant frequency of the signal, calculated by calculateFreqDelta.
float currentDomFreq = 0;

// Difference between the previous and the current dominant frequency of the signal, calculated by calculateFreqDelta.
float freqDelta = 0;

/*/
########################################################
    Stuff relavent to additive synthesizer
########################################################
/*/

EventDelay kChangeBinsDelay;
const unsigned int gainChangeMsec = 1000;

Oscil <2048, AUDIO_RATE> asinc(COS2048_DATA); //asinc is the carrier wave 20Hz. This is the sum of all other waves and the wave that should be outputted.
Oscil <2048, AUDIO_RATE> asin1(SIN2048_DATA); //20Hz
Oscil <2048, AUDIO_RATE> asin2(SIN2048_DATA); //30 - 39
Oscil <2048, AUDIO_RATE> asin3(SIN2048_DATA); //40 - 49
Oscil <2048, AUDIO_RATE> asin4(SIN2048_DATA); //50 - 59
//Oscil <2048, AUDIO_RATE> asin5(SIN2048_DATA); //60 - 69 // These are added for later, when changing the number of bins
//Oscil <2048, AUDIO_RATE> asind(SIN2048_DATA); //15-75Hz, full range for Nanolux code
// Oscil <2048, AUDIO_RATE> aModulator(SIN2048_DATA);
// Oscil <2048, CONTROL_RATE> kIntensityMod(SIN2048_DATA);

float smoothness = 0.05f;
Smooth <long> aSmoothIntensity(smoothness);

const int OscilCount = 5;  //The total number of waves. Modify this if more waves are added, or the program will segfault.

int nextAmplitudeGains[OscilCount];
int amplitudeGains[OscilCount];  //Set between 0 and 1.
int ampGainStep[OscilCount];
int waveFrequencies[OscilCount];

int updateCount = 0;
int sampleCount = 0;

unsigned int carrier;

// nanolux domFreq
int domFreq = 0;

/*/
########################################################
  Other
########################################################
/*/

int opmode = 0;

int toggle = 0;

int sampleAvg = 0;

/*/
########################################################
    Setup
########################################################
/*/

void setup() {
  // set baud rate
  Serial.begin(115200);
  
  // setup pins
  pinMode(AUDIO_INPUT_PIN, INPUT);

  for (int i = 0; i < OscilCount; i++) {
    amplitudeGains[i] = 0;
    nextAmplitudeGains[i] = 0;
    ampGainStep[i] = 0;
  }

  // Additive Synthesis
  asinc.setFreq(24);  //16 Hz - Base shaker sub-harmonic freq.
  asin1.setFreq(44);  //Set frequencies here. (Hardcoded for now)
  asin2.setFreq(67);
  asin3.setFreq(94);
  asin4.setFreq(120);
  //asind.setFreq(0);  //Nanolux dominant frequency value changes when enough samples are taken
  startMozzi(CONTROL_RATE);
  

  while (!Serial)
    ;
  Serial.println("Ready");
}

/*/
########################################################
    Loop
########################################################
/*/

void loop() {
  audioHook();
}

/*/
########################################################
    Additive Synthesis Functions
########################################################
/*/

void updateControl() {
  if (updateCount >= CONTROL_RATE) {
    updateCount = 0;
    sampleCount = 0;
  }

  /* Output string */
  // char buffer[500];
  // memset(buffer, 0, sizeof(buffer));

  if (updateCount == FADE_RATE * sampleCount) {
    sampleCount++;
    /* set number of bins and curve value */
  // while (Serial.available()) {
  //   char data = Serial.read();
  //   Serial.println("Enter integer for number of bins:");
  //   if (data == 'x') {
  //     while (Serial.available() == 1) {}
  //     bins = Serial.parseInt();
  //     Serial.println("Enter float for curve value:");
  //     while (Serial.available() == 1) {}
  //     curve = Serial.parseFloat();
  //     Serial.println("New bins and curve value set.");
  //     Serial.printf("bins: %d\tcurve: %.2f\n", bins, curve);
  //     delay(500);
  //   }
  // }

    sampleAvg = recordSample(FFT_WIN_SIZE, 4096, 0);
    sampleAvg = int(sampleAvg / FFT_WIN_SIZE);

    /* FFT */
    //FFT.DCRemoval();                                  // Remove DC component of signal
    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
    FFT.Compute(FFT_FORWARD);                         // Compute FFT
    FFT.ComplexToMagnitude();                         // Compute frequency magnitudes

    /* FFT to Bins */
    int averageBinsArray[bins];
    int sumOfAmps = SplitSample(vReal, FFT_WIN_SIZE >> 1, averageBinsArray, bins, curve);
    // float normalizedAvgBinsArray[bins];
    // normalizeArray(averageBinsArray, normalizedAvgBinsArray, bins);

    // mapping each average normalized bin from FFT analysis to a sine wave gain value for additive synthesis
    float domFreqAmp = 0.0;    // Nanolux dominant frequency amplitude
    float totalGains = 0.0;
    for (int i = 0; i < bins; i++) {
      amplitudeGains[i] = nextAmplitudeGains[i];
      // if the AUDIO_INPUT is relatively unchanged, the gain value is set to 0
      if (sampleRange < sampleRangeThreshold) {
        nextAmplitudeGains[i] = nextAmplitudeGains[i] >> 1;
      } else {
        int gainValue = map(averageBinsArray[i], 0, sumOfAmps, 0, 255);
        totalGains += gainValue;
        nextAmplitudeGains[i] = gainValue;
      }
      ampGainStep[i] = int((nextAmplitudeGains[i] - amplitudeGains[i]) / FADE_RATE);

      // form output fstring (commented out to reduce loop usage but can be uncommented for testing)
      //formBinsArrayString(amplitudeGains[i], waveFrequencies[i], bins, i, buffer, curve, ampGainStep[i]);
    }
    //amplitudeGains[5] = domFreqAmp;

    /* Nanolux Code Implementation */
    if (sampleBuffCount == freqAvgSampCount) {    // run nanolux freqDelta when the samples buffer is full
      sampleBuffCount = 0;
      //calculateFrequencyDelta();
      // Map the dominant frequency to a basshaker frequency
      //domFreq = map(currentDomFreq, 0, 5000, 15, 75);    
      //asind.setFreq(domFreq); // set the mapped frequency to associated wave
    }
    
    /* Serial Ouput */
    // char buffer3[128];
    // sprintf(buffer3, "Plotter: Min:%d\tMax:%d\tOutput(0-255):%03d\tInput(0-255):%03d\tDomFreq(15-75Hz):%d\n", 0, 255, carrier, map((long)sampleAvg, 0, sampleMax, 0, 255), domFreq);
    // strcat(buffer, buffer3);    

    asinc.setFreq(waveFrequencies[0]);
    asin1.setFreq(waveFrequencies[1]);
    asin2.setFreq(waveFrequencies[2]);
    asin3.setFreq(waveFrequencies[3]);
    asin4.setFreq(waveFrequencies[4]);

  }
  else {
    crossfadeAmplitudes();
    /* Serial Ouput */
    // char buffer3[128];
    // sprintf(buffer3, "Plotter: Min:%d\tMax:%d\tOutput(0-255):%03d\tInput(0-255):%03d\tDomFreq(15-75Hz):%d\n", 0, 255, carrier, map((long)sampleAvg, 0, sampleMax, 0, 255), domFreq);
    // strcat(buffer, buffer3);
  }
  //Serial.print(buffer);

  updateCount++;
}

void crossfadeAmplitudes() {
  for(int i = 0; i < bins; i++) {
    amplitudeGains[i] += ampGainStep[i];
  }
}

int updateAudio() {
  return
    int((amplitudeGains[0] * asinc.next()) + 
    (amplitudeGains[1] * asin1.next()) + 
    (amplitudeGains[2] * asin2.next()) + 
    (amplitudeGains[3] * asin3.next()) + 
    (amplitudeGains[4] * asin4.next())) >> 8;// + amplitudeGains[5] * asind.next();   //Additive synthesis equation
    // asind.setFreq(waveFrequencies[0]);
    // asin1.setFreq(waveFrequencies[1]);
    // asin2.setFreq(waveFrequencies[2]);
    // asin3.setFreq(waveFrequencies[3]);
    // asin4.setFreq(waveFrequencies[4]);

  //carrier = aSmoothIntensity(carrier);

  ///return carrier;
}

/*/
########################################################
    FFT to Bins Functions
########################################################
/*/

/* Splits the bufferSize into X groups, where X = splitInto
    the curveValue determines the curve to follow when buffer is split
    if curveValue = 1 : then buffer is split into X even groups
    0 < curveValue < 1 : then buffer is split into X groups, following a concave curve
    1 < curveValue < infinity : then buffer is split into X groups following a convex curve */
int SplitSample(float *data, uint16_t bufferSize, int *destArray, int splitInto, float curveValue) {            
  // (x/splitInto)^(1/curveValue)
  float step = 1.0 / splitInto;                         // how often to step on the x-axis to determine bin value
  float exponent = 1.0 / curveValue;                    // power of the parabolic curve (x^exponent)
  int ampSum = 0;
  float topOfSample = 0;                                // The frequency of the current amplitude
  int lastJ = 0;                                        // the last amplitude taken from vReal
  for (int i = 0; i < splitInto; i++) {                 // Calculate the size of each bin and the amplitudes into each bin
    float xStep = (i + 1) * step;                       // x-axis step
    // (x/splitInto)^(1/curveValue)
    int binSize = round(bufferSize * pow(xStep, exponent));
    int newJ = lastJ;
    int ampGroupSum = 0;
    int ampGroupCount = 0;
    int maxBinAmp = 0;
    int maxBinAmpFreq = 0;
    for (int j = lastJ; j < binSize; j++) {             // for the next group of amplitudes
      topOfSample = topOfSample + sFreqBySamples;       // calculate the associated frequency
      // if amplitude is above certain threshold, set it to -1 to exclude from signal processing
      if (data[j] > FFT_FLOOR_THRESH && topOfSample > 100) {
        int amp = int(data[j]);
        ampGroupSum += amp;
        ampGroupCount += 1;
        if (amp > maxBinAmp) {
          maxBinAmp = amp;
          maxBinAmpFreq = int(topOfSample);
        }
      }
      // Debugging stuff
      // Serial.printf("I: %d\tFreq: %.2f\tAmp: %.2f\t", j, topOfSample, vData[j]);
      // Serial.println();
      newJ += 1;
    }
    if (ampGroupCount > 1) {
      destArray[i] = int(ampGroupSum / ampGroupCount);
      waveFrequencies[i] = int(map(maxBinAmpFreq, 60, sFreqBy2, 20, 700));
    } else {
      destArray[i] = ampGroupSum;
    }
    ampSum += destArray[i];
    if (destArray[i] > maxAvgAmp) {
      maxAvgAmp = destArray[i];
    }
    lastJ = newJ;
    //waveFrequencies[i] = int(map(maxBinAmpFreq, 60, sFreqBy2, 19, 220));
  }
  return ampSum;
}

// gets average of an array
float getArrayMean(float *array, int arraySize) {
  float arraySum = 0.0;
  int n = 0;      // the number of valid values
  for (int i = 0; i < arraySize; i++) {
    // if the value in array is valid, use it to calculate the mean
    if (array[i] >= 0) {
      arraySum += array[i];
      n += 1;     // increment count
    }
  }
  if (n == 0) {   // if all values are invalid, return 0.0 as average
    return 0.0;
  }
  return (float)(arraySum / n); // otherwise return the average
}

// normalizes array and puts the normalized values in destArray
void normalizeArray(int *array, float *destArray, int arraySize) {
  //float min = getArrayMin(array, arraySize);
  //float max = getArrayMax(array, arraySize);
  float sum = 0;
  for (int i = 0; i < arraySize; i++) {
    sum += array[i];
  }
  //sum = sum + sum * 0.2; // leave 0.25 for ndf
  float iv = 0;     // initialization vector
  if (sum > 0) {
    iv = 1.0 / (sum + sum * 0.2);
  }
  for (int i = 0; i < arraySize; i++) {
    float normalizedValue = array[i] * iv;
    destArray[i] = normalizedValue;
  }
}

// returns the minimum value in array
float getArrayMin(float *array, int arraySize) {
  float min = array[0];
  for (int i = 1; i < arraySize; i++) {
    if (array[i] < min) {
      min = array[i];
    }
  }
  return min;
}

// returns the maximum value in array
float getArrayMax(float *array, int arraySize) {
  float max = array[0];
  for (int i = 1; i < arraySize; i++) {
    if (array[i] > max) {
      max = array[i];
    }
  }
  return max;
}

// calculate standard deviation : s = sqrt(sum((arr[i] - arr_mean)^2) / size)
float calculateStandardDeviation(float *array, int arraySize) {
  float average = getArrayMean(array, arraySize);
  float variance = 0;
  int n = 0;    // the number of valid values
  for(int i = 0; i < arraySize; i++) {
    if (array[i] >= 0) {
      variance += pow(array[i] - average, 2);
      n += 1;   // increment n
    }
  }
  if (n == 0) {
    return 0.0;
  }
  return (float)sqrt(variance / n);
}

// form bins array string
void formBinsArrayString(int binValue, int frequency, int arraySize, int i, char *destBuffer, float curveValue, int gainStep) {
  float step = 1.0 / arraySize;
  float parabolicCurve = 1.0 / curveValue;

  float xStep = i * step;
  char buffer[48];
  memset(buffer, 0, sizeof(buffer));
  int rangeLow = 0;
  if (i > 0) {
    rangeLow = round(sFreqBy2 * pow(xStep, parabolicCurve));
  }
  int rangeHigh = round(sFreqBy2 * pow(xStep + step, parabolicCurve));
  sprintf(buffer, "%d - %dHz: %03d for %03dHz, %03d\t", rangeLow, rangeHigh, binValue, frequency, gainStep);
  strcat(destBuffer, buffer);
}

/*/
########################################################
    Nanolux 
########################################################
/*/

// Calculates the change in dominant frequency of the signal, and stores the new result in currentDomFreq. The previous value is moved to previousDomFreq, and the
// difference between these values is stored in freqDelta. The function works by calculating the magnitudes of the FFT's of several sequential parts of the signal
// and averaging them, and then finding the frequency and value associated with the highest peak in this average. The value of the new peak must have a minimum
// magnitude (the threshhold for which is defined by []), and its frequency must be different from the previously recorded dominant frequency by a minimum amount
// (which is defined by []) in order for the new values of currentDomFreq, previousDomFreq, and freqDelta to be calculated.
void calculateFrequencyDelta() {
  // Calculates the magnitudes of the FFT's of each sequential segment of the signal.
  // Loops over each FFT window
  for (int i = 0; i < FREQ_AVG_WIN; i++) {
    // Loops over vReal, and sets each value equal to a sample from the sample buffer.
    // Also zeroes out vImag.
    for (int s = 0; s < FFT_WIN_SIZE; s++) {
      vReal[s] = samples[i * FFT_WIN_SIZE + s];
      vImag[s] = 0;
    }

    // Calculates magnitudes of the FFT of this segment of the signal.
    FFT.DCRemoval();                                  // Remove DC component of signal
    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
    FFT.Compute(FFT_FORWARD);                         // Compute FFT
    FFT.ComplexToMagnitude();                         // Compute frequency magnitudes

    // Moves the resulting frequency magnitudes in vReal into freqs[][], so that it can be averaged with the other windows later.
    for (int s = 0; s < FFT_WIN_SIZE / 2; s++) {
      freqs[i][s] = vReal[s];
    }
  }

  // Averages all of the FFT windows together across time, to get the average frequency composition of the signal over the past short period.
  // Adds the magnitudes of the frequencies stored in every frequency window onto the first first frequency window (freqs[0][...]), which results in the first
  // frequency window containing the sum of all of them.
  for (int i = 1; i < FREQ_AVG_WIN; i++) {
    for (int f = 0; f < FFT_WIN_SIZE / 2; f++) {
      freqs[0][f] += freqs[i][f];
    }
  }

  // Divides each value in the first frequency window by the number of frequency windows that are being averaged across, to yield the average frequency composition
  // of the signal.
  for (int i = 0; i < FFT_WIN_SIZE / 2; i++) {
    freqs[0][i] /= FREQ_AVG_WIN;
  }

  // Creates variables for temporarily storing the magnitude and frequency of the current dominant frequency of the signal.
  int majorPeakIdx = 0;
  float majorPeakVal = 0;

  // Loops over the first sample window, and finds the x and y values of its maximum. Skips the first value (i=0), because this corresponds to the DC component of
  // the signal (0 Hz), which we don't care about.
  for (int i = 1; i < FFT_WIN_SIZE / 2; i++) {
    // If the value of the peak at this location exceeds the value of the previously recorded maximum peak, store the value of this one instead.
    if (freqs[0][i] > majorPeakVal) {
      majorPeakVal = freqs[0][i];
      majorPeakIdx = i;
    }
  }

  // Checks if the newly recorded major peak exceeds the required significance threshhold, and if it varies from the previous dominant frequency by a meaningful amount.
  // If it meets both of these requirements, record the value into currentDomFreq as the new dominant frequency of the signal.
  if ((majorPeakVal > FREQ_SIG_THRESH) && (abs((majorPeakIdx * (SAMPLE_FREQ / FFT_WIN_SIZE)) - currentDomFreq) > FREQ_DELTA_ERROR_MARGIN)) {
    previousDomFreq = currentDomFreq;                              // Move the previously dominant frequency of the signal into previousDomFreq
    currentDomFreq = majorPeakIdx * (SAMPLE_FREQ / FFT_WIN_SIZE);  // Calculate and store the current dominant frequency of the signal in currentDomFreq
    freqDelta = currentDomFreq - previousDomFreq;                  // Calculate the difference between the current and previous dominant frequencies of the signal.
  }
}

// Records [count] samples into the array samples[] from the audio input pin, at a sampling frequency of SAMPLE_FREQ
int recordSample(int sampleCount, int currSampleMin, int currSampleMax) {
  if (sampleCount > 0) {
    //long int sampleTime = micros();
    int i = FFT_WIN_SIZE - sampleCount;
    int sample;
    sample = mozziAnalogRead(AUDIO_INPUT_PIN); // Read sample from input
    // store the maximum sample to account for various input voltages
    if (sample > currSampleMax) {
       currSampleMax = sample;
    } else if (sample < currSampleMin) {
      currSampleMin = sample;
    }
    vReal[i] = (float)sample;                 // For FFT to Bins code
    samples[sampleBuffCount] = (float)sample; // For Nanolux implementation
    sampleBuffCount++;
    vImag[i] = 0;                             // set imaginary values to 0
    //while (micros() - sampleTime < sampleDelayTime) {}
    return sample + recordSample(sampleCount - 1, currSampleMin, currSampleMax);
  }
  else {
    if (currSampleMax > sampleMax) {
      sampleMax = currSampleMax;
    }
    sampleRange = currSampleMax - currSampleMin;
    if (sampleRange > sampleRangeMax) {
      sampleRangeMax = sampleRange;
    } else if (sampleRange < sampleRangeMin) {
      sampleRangeMin = sampleRange;
    }
    sampleRangeThreshold = sampleRangeMin * 3;
  }
  return 0;
}