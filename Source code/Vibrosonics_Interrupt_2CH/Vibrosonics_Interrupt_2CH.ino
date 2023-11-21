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

  delay(4000);
}

/*/
########################################################
  Loop
########################################################
/*/

void loop() {
  if (v.ready()) {
    // loopt = micros();
    v.pullSamples();

    v.resume();

    //v.performFFT();

    // v.processData();

    v.resetAllWaves();

    int a = v.addWave(0, 50, 50);
    int d = v.addWave(1, 50, 20);
    int b = v.addWave(0, 10, 20);
    int c = v.addWave(0, 70, 30);

    //Serial.printf("%d, %d, %d, %d\n", a, b, c, d);

    v.removeWave(b);
    v.removeWave(a);
    //v.removeWave(6);
    
    //v.printWaves();

    v.generateAudioForWindow();
    // Serial.println(micros() - loopt);
  }
}