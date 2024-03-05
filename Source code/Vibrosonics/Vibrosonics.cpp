#include "Vibrosonics.h"
#include "FFTBuffer.h"
#include "Breadslicer.h"
#include "Pulse.h"


int* AudioLabInputBuffer = AudioLab.getInputBuffer();
// float* rawFFTData = getFFTData();

FFTBuffer FFTData = FFTBuffer(WINDOW_SIZE, SAMPLE_RATE, 8);
FFTBuffer FFTDataLow = FFTBuffer(WINDOW_SIZE, SAMPLE_RATE / 4, 2);

Breadslicer breadslicer = Breadslicer(WINDOW_SIZE, SAMPLE_RATE);

const int numBands = 3;
int breadslicerBands[numBands + 1] = { 100, 1500, 2500, 4096 };

int freqs[numBands] = { 18, 34, 55 };
int ampWeights[numBands] = { 2.0, 4.0, 15.0 };

Pulse pulses[8];

void Vibrosonics::init() {
  Serial.begin(115200);
  while (!Serial)
    ;
  delay(4000);

  breadslicer.setBands(breadslicerBands, numBands);

  // calculate sinc filter table for downsampling by a factor of 4 with 8 number of zeroes
  calculateDownsampleSincFilterTable(4, 8);

  // initialize AudioLab
  AudioLab.init();
}

float lowFreqsVolume = 0;

void Vibrosonics::update() {
  if (AudioLab.ready()) {
    //unsigned long timeMicros = micros();

    // dwnsampling for better bass
    if (downsampleSignal(AudioLabInputBuffer)) {
      float* rawFFTData = performFFT(getDownsampledSignal());

      FFTDataLow.pushData(rawFFTData);

      float* currentWindow = FFTDataLow.getWindow(FFTDataLow.getCurrentWindowIndex());

      lowFreqsVolume = getSum(currentWindow, WINDOW_SIZE_BY2);

      if (getMean(currentWindow, WINDOW_SIZE_BY2) > 15.0) {


        //doRMS(tempFreqs, WINDOW_SIZE_BY2, 4);

        float tempFreqs[WINDOW_SIZE_BY2];
        for (int i = 0; i < WINDOW_SIZE_BY2; i++) {
          tempFreqs[i] = currentWindow[i];
        }

        smartNoiseFloor(tempFreqs, WINDOW_SIZE_BY2, 1.3, 8);

        int numPeaks = 2;
        // use findPeaks function to find at most 16 peaks in frequency data
        findPeaks(tempFreqs, numPeaks);

        int peakFreqs[numPeaks];
        float peakAmps[numPeaks];
        for (int i = 0; i < numPeaks; i++) {
          peakFreqs[i] = FFTPeaks[0][i];
          peakAmps[i] = FFTPeaks[1][i];
        }

        mapAmplitudes(peakAmps, numPeaks, 3000);

        for (int i = 0; i < numPeaks; i++) {
          peakFreqs[i] = interpolateAroundPeak(currentWindow, peakFreqs[i], SAMPLE_RATE / 4);
        }

        for (int i = 0; i < numPeaks; i++) {
          if (peakFreqs[i] > 120) break;

          if (pulses[i].getPulseState() == READY) {
            pulses[i].setAttack(peakFreqs[i], peakAmps[i] * 0.8, 1);
            pulses[i].setSustain(peakFreqs[i], peakAmps[i] * 1.0, 3);
            pulses[i].setRelease(peakFreqs[i], peakAmps[i] * 0.2, 7);
            pulses[i].setReleaseCurve(1.5);
            pulses[i].start();
          }

          for (int j = 0; j < 8; j++) {
            if (pulses[j].getPulseState() == READY) {
              pulses[j].setAttack(peakFreqs[i], peakAmps[i] * 0.7, 2);
              pulses[j].setSustain(peakFreqs[i], peakAmps[i] * 1.0, 1);
              pulses[j].setRelease(peakFreqs[i], peakAmps[i] * 0.5, 3);
              pulses[j].setReleaseCurve(0.5);
              pulses[j].start();
              break;
            }
          }
        }
      }
    }

    // perform FFT on AudioLab input buffer, using ArduinoFFT
    float* rawFFTData = performFFT(AudioLabInputBuffer);

    // a debug print to compare raw fft output to downsampled output
    //Serial.printf("default peak: %d\n", FFTMajorPeak(SAMPLE_RATE));

    // store frequencies computed by FFT into FFT data buffer
    FFTData.pushData(rawFFTData);

    float* currentWindow = FFTData.getWindow(FFTData.getCurrentWindowIndex());
    float* previousWindow = FFTData.getWindow(FFTData.getCurrentWindowIndex() - 1);

    int bassIdx = round(200.0 * WINDOW_SIZE / SAMPLE_RATE);
    float tempFreqs[WINDOW_SIZE_BY2];
    //Serial.println();
    for (int i = 0; i < WINDOW_SIZE_BY2; i++) {
      //Serial.printf("%.2f, ", currentWindow[i]);
      tempFreqs[i] = i >= bassIdx ? currentWindow[i] : 0.0;
      //tempFreqs[i] = currentWindow[i];
      //Serial.printf("%.2f, ", tempFreqs[i]);
    }
    //Serial.println();


    // ensure that the mean energy of frequency magnitudes is above a certain threshold
    if (getMean(tempFreqs, WINDOW_SIZE_BY2) > 5.0) {

      //doRMS(tempFreqs, WINDOW_SIZE_BY2, 2);

      smartNoiseFloor(tempFreqs, WINDOW_SIZE_BY2, 0.6, 8);

      float changeMagnitude = 1.0;
      int deltaFreq = frequencyMaxAmplitudeDelta(currentWindow, previousWindow, 1000, 4096, changeMagnitude) * SAMPLE_RATE / WINDOW_SIZE;

      int bandIdx = -1;
      for (int i = 0; i < numBands; i++) {
        if (deltaFreq >= breadslicerBands[i] && deltaFreq <= breadslicerBands[i + 1]) {
          bandIdx = i;
          break;
        }
      }

      breadslicer.perform(tempFreqs);

      float* breadslicerOutput = breadslicer.getOutput();
      float amps[numBands];


      for (int i = 0; i < numBands; i++) {
        amps[i] = breadslicerOutput[i] * ampWeights[i];
        if (i == bandIdx) { 
          amps[i] * 25.0;
          //Serial.println("yes");
        }
        Serial.print(amps[i]);
        Serial.print(" ");
      }
      Serial.println();

      // for (int i = 0; i < numBands; i++) {
      //   if (i != _maxIdx) amps[i] = 0.0;
      //   Serial.print(amps[i]);
      //   Serial.print(" ");
      // }
      // Serial.println();

      // maps amplitudes to 0-127 range by dividing amplitudes by the max amplitude sum
      // (will divide by actual sum if it exceeds this threshold)

      int _maxAmpSum = 3000;
      if (lowFreqsVolume > 1500) _maxAmpSum = 1000;
      mapAmplitudes(amps, numBands, _maxAmpSum);

      assignWaves(freqs, amps, numBands, 1, 5, 140);
    }


    // call static function update() to update all pulses every window
    Pulse::update();
    // call AudioLab.synthesize() after all waves are set.
    AudioLab.synthesize();
    //AudioLab.printWaves();

    //Serial.println(micros() - timeMicros);
  }
}