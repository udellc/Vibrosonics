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

hw_timer_t *SAMPLING_TIMER = NULL;

void stopISR() {
  timerAlarmDisable(SAMPLING_TIMER);                 // disable interrupt
  timerDetachInterrupt(SAMPLING_TIMER);
  timerRestart(SAMPLING_TIMER);
}

void startISR(const unsigned long interval_us, void(*fnPtr)()) {
  SAMPLING_TIMER = timerBegin(0, 80, true); 
  timerAttachInterrupt(SAMPLING_TIMER, fnPtr, true);
  timerAlarmWrite(SAMPLING_TIMER, interval_us, true);
  timerAlarmEnable(SAMPLING_TIMER);                 // enabled interrupt
}

Vibrosonics v(startISR, stopISR);

void setup() {
  // set baud rate
  Serial.begin(115200);
  // wait for Serial to set up
  while (!Serial);

  delay(4000);

  v.init();
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

    //v.processData();
    
    //v.printWaves();

    v.generateAudioForWindow();
    Serial.println(micros() - loopt);
  }
}