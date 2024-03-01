#include "Vibrosonics.h"
#include "Pulse.h"


int* AudioLabInputBuffer = AudioLab.getInputBuffer();

// Pulse pulseMajorPeak = Pulse(0, SINE);
// Pulse pulseDeltaAmp = Pulse(0, SINE);
// Pulse pulseBass = Pulse(1, SINE);

void Vibrosonics::init() {
  Serial.begin(115200);
  while (!Serial)
    ;
  delay(4000);

  // calculate sinc filter table for downsampling by a factor of 4 with 8 number of zeroes
  calculateDownsampleSincFilterTable(4, 8);

  // initialize AudioLab
  AudioLab.init();
}

void Vibrosonics::update() {
  if (AudioLab.ready()) {
    //unsigned long timeMicros = micros();

    // perform FFT on AudioLab input buffer, using ArduinoFFT
    performFFT(AudioLabInputBuffer);

    // a debug print to compare raw fft output to downsampled output
    //Serial.printf("default peak: %d\n", FFTMajorPeak(SAMPLE_RATE));

    // store frequencies computed by FFT into freqs array
    storeFFTData();

    // ensure that the mean energy of frequency magnitudes is above a certain threshold
    if (getMean(freqsCurrent, WINDOW_SIZE) > 15.0) {
      
      // floor frequency magnitudes below a certain threshold
      //noiseFloor(freqsCurrent, 15.0);

      // use findPeaks function to find at most 16 peaks in frequency data
      findPeaks(freqsCurrent, 16);

      // maps amplitudes to 0-127 range by dividing amplitudes by the max amplitude sum 
      // (will divide by actual sum if it exceeds this threshold)
      // mapAmplitudes(FFTPeaksAmp, MAX_NUM_PEAKS, 3000);

      int peakFreqs[16];
      float peakAmps[16];
      for (int i = 0; i < 16; i++) {
        peakFreqs[i] = FFTPeaks[0][i];
        peakAmps[i] = FFTPeaks[1][i];
      }

      mapAmplitudes(peakAmps, 16, 3000);

      for (int i = 0; i < 16; i++) {
        peakFreqs[i] = interpolateAroundPeak(freqsCurrent, peakFreqs[i]);
      }

      assignWaves(peakFreqs, peakAmps, 16, 1);

    }

    // call static function update() to update all pulses every window
    //Pulse::update();
    // call AudioLab.synthesize() after all waves are set.
    AudioLab.synthesize();
    AudioLab.printWaves();

    //Serial.println(micros() - timeMicros);
  }
}