/*/
########################################################
  This code asynchronously samples a signal via an interrupt timer to fill an input buffer. Once the input buffer is full, a fast fourier transform is performed using the arduinoFFT library.
  The data from FFT is then processed and a new signal is formed using a sine wave table and audio synthesis, which fills an output buffer that is then outputted asynchronously by the same
  interrupt timer used for sampling the signal.
########################################################
/*/

#include "Vibrosonics.h"

Vibrosonics v;

void setup() {
  // set baud rate
  Serial.begin(115200);
  // wait for Serial to set up
  while (!Serial);

  delay(3000);

  v.init();
}

/*/
########################################################
  Loop
########################################################
/*/

unsigned long loopt = 0;


void loop() {
  // returns true when next window can be synthesized
  if (v.ready()) {
    #ifdef DEBUG
    //v.disableAudio();
    loopt = micros();
    #endif

    // pull samples from audio input buffer
    v.pullSamples();

    // continue sampling
    v.flush();

    // process audio input data (fft, major peaks, frequency of max change) and assign sine waves
    v.processData();

    // generates a window of samples for audio output buffer
    v.generateAudioForWindow();

    #ifdef DEBUG
    Serial.println(micros() - loopt);
    v.printWaves();
    // v.enableAudio();
    #endif
  }
}