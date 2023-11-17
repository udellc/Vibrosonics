#include "Vibrosonics.h"

Vibrosonics::Vibrosonics() {

}
void Vibrosonics::init(void) {
  analogReadResolution(12);

  // setup pins
  pinMode(AUD_OUT_PIN_L, OUTPUT);
  pinMode(AUD_OUT_PIN_R, OUTPUT);

  // initialize generate audio and output buffers to 0, not necassary but helps prevent glitches when program boots
  initAudio();
  // attach timer interrupt
  setupISR();

  disableAudio();
  Serial.println("Vibrosonics setup complete");
  Serial.printf("SAMPLE RATE: %d Hz\tWINDOW SIZE: %d\tSPEED AND RESOLUTION: %.1f Hz\tTIME PER WINDOW: %.1f ms\tAUD IN OUT DELAY: %.1f ms", SAMPLING_FREQ, FFT_WINDOW_SIZE, freqRes, sampleDelayTime * FFT_WINDOW_SIZE * 0.001, 2 * sampleDelayTime * FFT_WINDOW_SIZE * 0.001);
  Serial.println();
  enableAudio();
}

bool Vibrosonics::ready(void) {
  if (AUD_IN_BUFFER_FULL()) return 1;
  return 0;
}

void Vibrosonics::resume(void) {
  // reset AUD_IN_BUFFER_IDX to 0, so interrupt can continue to perform audio input and output
  RESET_AUD_IN_OUT_IDX();
}