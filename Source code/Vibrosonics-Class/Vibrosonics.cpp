#include "Vibrosonics.h"

hw_timer_t* Vibrosonics::SAMPLING_TIMER = NULL;
volatile int Vibrosonics::AUD_IN_BUFFER[AUD_IN_BUFFER_SIZE];
volatile int Vibrosonics::AUD_OUT_BUFFER[NUM_OUT_CH][AUD_OUT_BUFFER_SIZE];
volatile int Vibrosonics::AUD_IN_BUFFER_IDX;
volatile int Vibrosonics::AUD_OUT_BUFFER_IDX;

void IRAM_ATTR Vibrosonics::AUD_IN_OUT(void) {
  if (AUD_IN_BUFFER_FULL()) return;

  int AUD_OUT_IDX = AUD_OUT_BUFFER_IDX + AUD_IN_BUFFER_IDX;
  dacWrite(AUD_OUT_PIN_L, AUD_OUT_BUFFER[0][AUD_OUT_IDX]);
  #if NUM_OUT_CH == 2
  dacWrite(AUD_OUT_PIN_R, AUD_OUT_BUFFER[1][AUD_OUT_IDX]);
  #endif

  #ifdef USE_FFT
  AUD_IN_BUFFER[AUD_IN_BUFFER_IDX] = adc1_get_raw(AUD_IN_PIN);
  #endif
  AUD_IN_BUFFER_IDX += 1;
}

bool Vibrosonics::AUD_IN_BUFFER_FULL(void) {
  return !(AUD_IN_BUFFER_IDX < AUD_IN_BUFFER_SIZE);
}

void Vibrosonics::RESET_AUD_IN_OUT_IDX(void) {
  AUD_OUT_BUFFER_IDX += AUD_IN_BUFFER_SIZE;
  if (AUD_OUT_BUFFER_IDX >= AUD_OUT_BUFFER_SIZE) AUD_OUT_BUFFER_IDX = 0;
  AUD_IN_BUFFER_IDX = 0;
}

void Vibrosonics::init(void) {
  // setup pins
  pinMode(AUD_OUT_PIN_L, OUTPUT);
  pinMode(AUD_OUT_PIN_R, OUTPUT);
  // setup adc
  adc1_config_width(ADC_WIDTH_12Bit);
  adc1_config_channel_atten(AUD_IN_PIN, ADC_ATTEN_11db);
  //adc_set_clk_div(1);

  // initialize generate audio and output buffers to 0, not necassary but helps prevent glitches when program boots
  initAudio();
  // setup timer interrupt
  setupISR();

  delay(1000);
  Serial.println("Vibrosonics setup complete");
  Serial.printf("SAMPLE RATE: %d Hz\tWINDOW SIZE: %d\tSPEED AND RESOLUTION: %.1f Hz\tTIME PER WINDOW: %.1f ms\tAUD IN OUT DELAY: %.1f ms", SAMPLING_FREQ, FFT_WINDOW_SIZE, freqRes, sampleDelayTime * FFT_WINDOW_SIZE * 0.001, 2 * sampleDelayTime * FFT_WINDOW_SIZE * 0.001);
  Serial.println();
  delay(1000);
  enableAudio();
}

bool Vibrosonics::ready(void) {
  return AUD_IN_BUFFER_FULL();
}

void Vibrosonics::flush(void) {
  // reset AUD_IN_BUFFER_IDX to 0, so interrupt can continue to perform audio input and output
  RESET_AUD_IN_OUT_IDX();
}

void Vibrosonics::enableAudio(void) {
  timerAlarmEnable(SAMPLING_TIMER);   // enable interrupt
}

void Vibrosonics::disableAudio(void) {
  timerAlarmDisable(SAMPLING_TIMER);  // disable interrupt
}

void Vibrosonics::setupISR(void) {
  // setup timer interrupt
  SAMPLING_TIMER = timerBegin(0, 80, true);             // setting clock prescaler 1MHz (80MHz / 80)
  timerAttachInterrupt(SAMPLING_TIMER, &AUD_IN_OUT, true); // attach interrupt function
  timerAlarmWrite(SAMPLING_TIMER, sampleDelayTime, true);     // trigger interrupt every @sampleDelayTime microseconds
  // timerAlarmEnable(SAMPLING_TIMER);                 // enabled interrupt
}

void Vibrosonics::initAudio(void) {
  // calculate values of cosine and sine wave at certain sampling frequency
  calculate_windowing_wave();
  calculate_waves();

  // initialize generate audio and output buffers to 0, not necassary but helps prevent glitches when program boots
  for (int i = 0; i < GEN_AUD_BUFFER_SIZE; i++) {
    for (int c = 0; c < NUM_OUT_CH; c++) {
    generateAudioBuffer[c][i] = 0.0;
      if (i < AUD_OUT_BUFFER_SIZE) {
        AUD_OUT_BUFFER[c][i] = 128;
        if (i < AUD_IN_BUFFER_SIZE) {
          AUD_IN_BUFFER[i] = 2048;
        }
      }
    }
  }
  AUD_IN_BUFFER_IDX = 0;
  AUD_OUT_BUFFER_IDX = 0;

  // reset waves
  resetAllWaves();
}