#include "Vibrosonics.h"
#include "Pulse.h"

int* AudioLabInputBuffer = AudioLab.getInputBuffer();

// example pulse
Pulse highPulse = Pulse(0, TRIANGLE);
Pulse lowPulse = Pulse(0, SINE);

void Vibrosonics::init() {
  Serial.begin(115200);
  while (!Serial)
    ;
  delay(4000);

  AudioLab.init();

  // high pulse (snare, hihats, etc) parameters
  highPulse.setAttack(200, 10, 0);
  highPulse.setAttackCurve(4.0);
  highPulse.setSustain(88, 100, 1);
  highPulse.setRelease(40, 0, 8);
  highPulse.setReleaseCurve(0.5);

  // low pulse (kick, impacts, etc) parameters
  lowPulse.setAttack(50, 10, 0);
  lowPulse.setAttackCurve(4.0);
  lowPulse.setSustain(80, 100, 4);
  lowPulse.setRelease(0, 0, 12);
  lowPulse.setReleaseCurve(0.5);
}

void Vibrosonics::update() {
  if (AudioLab.ready()) {

    // perform FFT on AudioLab input buffer, using ArduinoFFT
    performFFT(AudioLabInputBuffer);

    // store frequencies computed by FFT into freqs array
    storeFFTData();

    // call start() to pulse, note: this doesn't have to wait for a pulse to finish,
    // can call start() again to restart the pulse...

    if(detectPercussionFromTotalAmp(freqsCurrent, 0, 127, 6000) &&
        detectPercussionFromDeltaAmp(freqsCurrent, freqsPrevious, 0, 127, 3000)){
      lowPulse.start();
    }

    // ensure that the mean energy of frequency magnitudes is above a certain threshold
    if (getMean(freqsCurrent, WINDOW_SIZE) > 15.0) {
      
      // floor frequency magnitudes below a certain threshold
      noiseFloor(freqsCurrent, 15.0);

      // use findPeaks function to find at most 16 peaks in frequency data
      findPeaks(freqsCurrent, 16);

      // maps amplitudes to 0-127 range by dividing amplitudes by the max amplitude sum 
      // (will divide by actual sum if it exceeds this threshold)
      mapAmplitudes(FFTPeaksAmp, MAX_NUM_PEAKS, 3000);

      // assign peaks to sine waves
      assignWaves(FFTPeaks, FFTPeaksAmp, MAX_NUM_PEAKS);
    }

    // call static function update() to update all pulses every window
    Pulse::update();

    // call AudioLab.synthesize() after all waves are set.
    AudioLab.synthesize();
    //AudioLab.printWaves();
  }
}