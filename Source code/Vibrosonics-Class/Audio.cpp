#include "Vibrosonics.h"

// maps amplitudes between 0 and 127 range to correspond to 8-bit (0, 255) DAC on ESP32 Feather mapping is based on the MAX_AMP_SUM
void Vibrosonics::mapAmplitudes(int minSum) {
  // map amplitudes on both channels
  for (int c = 0; c < NUM_OUT_CH; c++) {
    int ampSum = 0;
    for (int i = 0; i < MAX_NUM_WAVES; i++) {
      if (waves_map[i].valid == 0) continue;
      if (waves_map[i].ch != c) continue;
      ampSum += waves_map[i].w.amp;
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
    for (int i = 0; i < MAX_NUM_WAVES; i++) {
      if (waves_map[i].valid == 0) continue;
      if (waves_map[i].ch != c) continue;
      int amplitude = waves_map[i].w.amp;
      if (amplitude == 0) continue;
      waves_map[i].w.amp = round(amplitude * divideBy * 127.0);
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
    for (int i = 0 ; i < MAX_NUM_WAVES; i++) {
      waves[c][i] = wave();
    }
  }
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
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