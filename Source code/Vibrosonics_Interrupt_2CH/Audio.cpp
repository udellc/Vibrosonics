#include "Vibrosonics.h"
#include <soc/sens_reg.h>
#include <soc/sens_struct.h>

// maps amplitudes between 0 and 127 range to correspond to 8-bit (0, 255) DAC on ESP32 Feather mapping is based on the MAX_AMP_SUM
void Vibrosonics::mapAmplitudes(int minSum) {
  // map amplitudes on both channels
  for (int c = 0; c < NUM_OUT_CH; c++) {
    int ampSum = 0;
    for (int i = 0; i < num_waves[c]; i++) {
      int amplitude = waves[c][i].amp;
      if (amplitude == 0) continue;
      ampSum += amplitude;
    }
    // since all amplitudes are 0, then there is no need to map between 0-127 range.
    if (ampSum == 0) { 
      resetWaves(c);
      continue; 
    }
    // value to map amplitudes between 0.0 and 1.0 range, the MAX_AMP_SUM will be used to divide unless totalEnergy exceeds this value
    float divideBy = 1.0 / float(ampSum > MAX_AMP_SUM ? ampSum : MAX_AMP_SUM);
    ampSum = 0;
    // map all amplitudes between 0 and 128
    for (int i = 0; i < num_waves[c]; i++) {
      int amplitude = waves[c][i].amp;
      if (amplitude == 0) continue;
      waves[c][i].amp = round(amplitude * divideBy * 127.0);
      ampSum += amplitude;
    }
    // ensures that nothing gets synthesized for this channel
    if (ampSum == 0) resetWaves(c);
  }
}

float Vibrosonics::get_wave_val(wave w) {
  float sin_wave_freq_idx = (sin_wave_idx * w.freq + w.phase) * _SAMPLING_FREQ;
  int sin_wave_position = (sin_wave_freq_idx - floor(sin_wave_freq_idx)) * SAMPLING_FREQ;
  return w.amp * sin_wave[sin_wave_position];
}

float Vibrosonics::get_sum_of_channel(int ch) {
  float sum = 0.0;
  for (int s = 0; s < num_waves[ch]; s++) {
    if (waves[ch][s].amp == 0 || (waves[ch][s].freq == 0 && waves[ch][s].phase == 0)) continue;
    sum += get_wave_val(waves[ch][s]);
  }
  return sum;
}

void Vibrosonics::calculate_windowing_wave(void) {
  float resolution = float(2.0 * PI / AUD_OUT_BUFFER_SIZE);
  for (int i = 0; i < AUD_OUT_BUFFER_SIZE; i++) {
    cos_wave_w[i] = 0.5 * (1.0 - cos(float(resolution * i)));
  }
}

void Vibrosonics::calculate_waves(void) {
  float resolution = float(2.0 * PI / SAMPLING_FREQ);
  // float tri_wave_step = 4.0 / SAMPLING_FREQ;
  // float saw_wave_step = 1.0 / SAMPLING_FREQ;
  for (int x = 0; x < SAMPLING_FREQ; x++) {
    sin_wave[x] = sin(float(resolution * x));
    // cos_wave[x] = cos(float(resolution * x));
    // tri_wave[x] = x <= SAMPLING_FREQ / 2 ? x * tri_wave_step - 1.0 : 3.0 - x * tri_wave_step;
    // sqr_wave[x] = x <= SAMPLING_FREQ / 2 ? 1.0 : 0.0;
    // saw_wave[x] = x * saw_wave_step;
  }
}

void Vibrosonics::generateAudioForWindow(void) {
  // setup waves array
  for (int c = 0; c < NUM_OUT_CH; c++) {
    num_waves[c] = 0;
  }
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    for (int c = 0; c < NUM_OUT_CH; c++) {
      waves[c][i] = wave();
    }
    if (waves_map[i].valid == 0) continue;
    int ch = waves_map[i].ch;
    waves[ch][num_waves[ch]++] = waves_map[i].w;
  }

  // calculate values for waves
  for (int i = 0; i < AUD_OUT_BUFFER_SIZE; i++) {
    // sum together the sine waves for left channel and right channel
    for (int c = 0; c < NUM_OUT_CH; c++) {
      // add windowed value to the existing values in scratch pad audio output buffer at this moment in time
      generateAudioBuffer[c][generateAudioIdx] += get_sum_of_channel(c) * cos_wave_w[i];
    }

    // copy final, synthesized values to volatile audio output buffer
    if (i < AUD_IN_BUFFER_SIZE) {
    // shifting output by 128.0 for ESP32 DAC, min max ensures the value stays between 0 - 255 to ensure clipping won't occur
      for (int c = 0; c < NUM_OUT_CH; c++) {
        AUD_OUT_BUFFER[c][generateAudioOutIdx] = max(0, min(255, int(round(generateAudioBuffer[c][generateAudioIdx] + 128.0))));
      }
      generateAudioOutIdx += 1;
      if (generateAudioOutIdx == AUD_OUT_BUFFER_SIZE) generateAudioOutIdx = 0;
    }

    // increment generate audio index
    generateAudioIdx += 1;
    if (generateAudioIdx == GEN_AUD_BUFFER_SIZE) generateAudioIdx = 0;
    // increment sine wave index
    sin_wave_idx += 1;
    if (sin_wave_idx == SAMPLING_FREQ) sin_wave_idx = 0;
  } 

  // reset the next window to synthesize new signal
  int generateAudioIdxCpy = generateAudioIdx;
  for (int i = 0; i < AUD_IN_BUFFER_SIZE; i++) {
    for (int c = 0; c < NUM_OUT_CH; c++) {
      generateAudioBuffer[c][generateAudioIdxCpy] = 0.0;
    }
    generateAudioIdxCpy += 1;
    if (generateAudioIdxCpy == GEN_AUD_BUFFER_SIZE) generateAudioIdxCpy = 0;
  }
  // determine the next position in the sine wave table, and scratch pad audio output buffer to counter phase cosine wave
  generateAudioIdx = int(generateAudioIdx - FFT_WINDOW_SIZE + GEN_AUD_BUFFER_SIZE) % int(GEN_AUD_BUFFER_SIZE);
  sin_wave_idx = int(sin_wave_idx - FFT_WINDOW_SIZE + SAMPLING_FREQ) % int(SAMPLING_FREQ);
}

void Vibrosonics::ON_SAMPLING_TIMER(void) {
  AUD_IN_OUT();
}

bool Vibrosonics::AUD_IN_BUFFER_FULL(void) {
  return !(AUD_IN_BUFFER_IDX < AUD_IN_BUFFER_SIZE);
}

void Vibrosonics::RESET_AUD_IN_OUT_IDX(void) {
  AUD_OUT_BUFFER_IDX += AUD_IN_BUFFER_SIZE;
  if (AUD_OUT_BUFFER_IDX >= AUD_OUT_BUFFER_SIZE) AUD_OUT_BUFFER_IDX = 0;
  AUD_IN_BUFFER_IDX = 0;
}

void Vibrosonics::AUD_IN_OUT(void) {
  if (AUD_IN_BUFFER_FULL()) return;

  int AUD_OUT_IDX = AUD_OUT_BUFFER_IDX + AUD_IN_BUFFER_IDX;
  dacWrite(AUD_OUT_PIN_L, AUD_OUT_BUFFER[0][AUD_OUT_IDX]);
  #if NUM_OUT_CH == 2
  dacWrite(AUD_OUT_PIN_R, AUD_OUT_BUFFER[1][AUD_OUT_IDX]);
  #endif

  #ifdef USE_FFT
  //adc1_get_raw(AUD_IN_PIN);
  //analogRead(A2);
  //AUD_IN_BUFFER[AUD_IN_BUFFER_IDX] = 2048;
  //AUD_IN_BUFFER[AUD_IN_BUFFER_IDX] = adc1_get_raw(AUD_IN_PIN);
  #endif
  AUD_IN_BUFFER_IDX += 1;
}

void Vibrosonics::enableAudio(void) {
  timerAlarmEnable(SAMPLING_TIMER);   // enable interrupt
}

void Vibrosonics::disableAudio(void) {
  timerAlarmDisable(SAMPLING_TIMER);  // disable interrupt
}

void Vibrosonics::setupISR(void) {
  // setup timer interrupt for audio sampling
  SAMPLING_TIMER = timerBegin(0, 80, true);             // setting clock prescaler 1MHz (80MHz / 80)
  timerAttachInterrupt(SAMPLING_TIMER, &ON_SAMPLING_TIMER, true); // attach interrupt function
  timerAlarmWrite(SAMPLING_TIMER, sampleDelayTime, true);     // trigger interrupt every @sampleDelayTime microseconds
  timerAlarmEnable(SAMPLING_TIMER);                 // enabled interrupt
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

  // reset waves
  resetAllWaves();
}

// int Vibrosonics::local_adc1_read(adc1_channel_t channel) {
//   u_int16_t adc_value;
//   SENS.sar_meas_start1.sar1_en_pad = (1 << channel);
//   while (SENS.sar_slave_addr1.meas_status != 0);
//   SENS.sar_meas_start1.meas1_start_sar = 0;
//   SENS.sar_meas_start1.meas1_start_sar = 1;
//   while(SENS.sar_meas_start1.meas1_done_sar == 0);
//   adc_value =  SENS.sar_meas_start1.meas1_data_sar;
//   return adc_value;
// }