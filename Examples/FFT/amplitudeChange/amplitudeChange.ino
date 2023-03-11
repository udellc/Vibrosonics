#include <arduinoFFT.h>
#include <Arduino.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}

// find all the major peaks in the fft spectrogram
void findMajorPeaks() {
  int majorPeaksCounter = 0;
  int arraySize = FFT_WIN_SIZE >> 1;
  int peakReached = 0;
  for (int i = 1; i < arraySize; i++) { 
    if (vReal[i - 1] > OUTLIER) {
      continue;
    }
    // get the change between the next and last index
    float change = vReal[i] - vReal[i - 1];
    // if change is positive, consider this is a rising peak
    if (change > 0) {
      peakReached = 0;
    } else {
      if (peakReached = 0) {
        peakIndexes[majorPeaksCounter] = i - 1;
        peakAmplitudes[majorPeaksCounter] = vReal[i - 1];
        peakReached = 1;
      }
    }
  }
}