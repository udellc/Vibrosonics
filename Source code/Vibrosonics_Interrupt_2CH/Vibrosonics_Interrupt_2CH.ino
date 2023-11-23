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

  //setupISR();
  //v.disableAudio();
  delay(1000);
}

/*/
########################################################
  Loop
########################################################
/*/

void loop() {
  if (v.ready()) {
    loopt = micros();
    v.pullSamples();

    v.resume();

    //v.performFFT();

    // v.processData();

    v.resetAllWaves();

    int a = v.addWave(0, 50, 50);
    int b = v.addWave(0, 10, 20);
    int c = v.addWave(0, 70, 30);
    int d = v.addWave(1, 50, 20);
    v.addWave(1, 60, 20);
    v.addWave(1, 70, 20);
    v.addWave(1, 80, 20);
    v.addWave(1, 90, 20);
    //v.addWave(1, 90, 20);

    //Serial.printf("%d, %d, %d, %d\n", a, b, c, d);

    //v.removeWave(b);
    //v.removeWave(a);
    
    //v.printWaves();

    v.generateAudioForWindow();

    //v.printWavesB();
    Serial.println(micros() - loopt);
  }
}

// void setupISR(void) {
//   // setup timer interrupt for audio sampling
//   SAMPLING_TIMER = timerBegin(0, 80, true);             // setting clock prescaler 1MHz (80MHz / 80)
//   timerAttachInterrupt(SAMPLING_TIMER, &v.ON_SAMPLING_TIMER, true); // attach interrupt function
//   timerAlarmWrite(SAMPLING_TIMER, sampleDelayTime, true);     // trigger interrupt every @sampleDelayTime microseconds
//   timerAlarmEnable(SAMPLING_TIMER);                 // enabled interrupt
// }