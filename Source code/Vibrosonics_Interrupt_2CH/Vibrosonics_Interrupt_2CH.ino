/*/
########################################################
  This code asynchronously samples a signal via an interrupt timer to fill an input buffer. Once the input buffer is full, a fast fourier transform is performed using the arduinoFFT library.
  The data from FFT is then processed and a new signal is formed using a sine wave table and audio synthesis, which fills an output buffer that is then outputted asynchronously by the same
  interrupt timer used for sampling the signal.

  Code written by: Mykyta "Nick" Synytsia - synytsim@oregonstate.edu
  
  Special thanks to: Vincent Vaughn, Chet Udell, and Oleksiy Synytsia
########################################################
/*/

#include "Vibrosonics.h"

unsigned long loopt;

Vibrosonics v;

void setup() {
  // set baud rate
  Serial.begin(115200);
  // wait for Serial to set up
  while (!Serial);

  delay(4000);

  v.init();

  delay(1000);
}

/*/
########################################################
  Loop
########################################################
/*/

void loop() {
  if (v.ready()) {
    //loopt = micros();
    v.pullSamples();

    v.resume();

    v.performFFT();

    // fft, data processing and sine wave assignment
    v.processData();

    v.resetSinWaves(0);
    // for (int i = 0; i < 8; i++) {
    //   v.addSinWave(i * 2 + 1, i * 5 + 1, i % 1, i * 5);
    // }
    v.addSinWave(10, 20, 0);

    v.setPhase(50, 0, 0);

    // generate audio for the next audio window
    v.generateAudioForWindow();
    //Serial.println(micros() - loopt);
  }
}

