#include "Vibrosonics.h"

Vibrosonics::Vibrosonics(void(*audStart)(const unsigned long interval_us, void(*fnPtr)()), void(*audStop)()) {
  ISRStart = audStart;
  ISRStop = audStop;
}

void Vibrosonics::enableAudio(void) {
  (*ISRStart)(sampleDelayTime, AUD_IN_OUT);
}

void Vibrosonics::disableAudio(void) {
  (*ISRStop)();
}

void Vibrosonics::init(void) {
  analogReadResolution(12);

  // setup pins
  //pinMode(AUD_IN_PINA, INPUT);
  pinMode(AUD_OUT_PIN_L, OUTPUT);
  pinMode(AUD_OUT_PIN_R, OUTPUT);
  pinMode(A2, INPUT);

  delay(1000);

  //adc1_config_width(ADC_WIDTH_BIT_12);
  //adc1_config_channel_atten(AUD_IN_PIN, ADC_ATTEN_DB_0);

  delay(1000);

  // initialize generate audio and output buffers to 0, not necassary but helps prevent glitches when program boots
  initAudio();
  // attach timer interrupt
  enableAudio();

  disableAudio();
  Serial.println("Vibrosonics setup complete");
  Serial.printf("SAMPLE RATE: %d Hz\tWINDOW SIZE: %d\tSPEED AND RESOLUTION: %.1f Hz\tTIME PER WINDOW: %.1f ms\tAUD IN OUT DELAY: %.1f ms", SAMPLING_FREQ, FFT_WINDOW_SIZE, freqRes, sampleDelayTime * FFT_WINDOW_SIZE * 0.001, 2 * sampleDelayTime * FFT_WINDOW_SIZE * 0.001);
  Serial.println();
  delay(1000);
  enableAudio();
}

bool Vibrosonics::ready(void) {
  return AUD_IN_BUFFER_FULL();
}

void Vibrosonics::resume(void) {
  // reset AUD_IN_BUFFER_IDX to 0, so interrupt can continue to perform audio input and output
  RESET_AUD_IN_OUT_IDX();
}