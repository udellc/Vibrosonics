#include "Vibrosonics.h"

/*/
########################################################
  Functions related to generating values for the circular audio output buffer
########################################################
/*/

bool Vibrosonics::addSinWave(int freq, int amp, int ch, int phase) {
  if (freq < 0 || phase < 0) {
    #ifdef DEBUG
    Serial.println("CANNOT ADD WAVE, FREQ AND PHASE MUST BE POSTIVE!");
    #endif
    return 0;
  } 
  
  if (!isValidChannel(ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT ADD WAVE");
    #endif
    return 0;
  }

  // ensures that the sum of waves in the channels is no greater than MAX_NUM_WAVES
  int num_waves_count = 0;
  for (int c = 0; c < AUD_OUT_CH; c++) {
    num_waves_count += num_waves[c];
  }

  if (num_waves_count == MAX_NUM_WAVES) {
    #ifdef DEBUG
    Serial.println("CANNOT ADD WAVE, CHANGE MAX_NUM_WAVES!");
    #endif
    return 0;
  }

  waves[ch][num_waves[ch]].amp = amp;
  waves[ch][num_waves[ch]].freq = freq;
  waves[ch][num_waves[ch]].phase = phase;
  num_waves[ch] += 1;
  return 1;
}

bool Vibrosonics::removeSinWave(int idx, int ch) {
  if (!isValidIndex(idx, ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT REMOVE WAVE");
    #endif
    return 0;
  }

  for (int i = idx; i < --num_waves[ch]; i++) {
    waves[ch][i].amp = waves[ch][i + 1].amp;
    waves[ch][i].freq = waves[ch][i + 1].freq;
    waves[ch][i].phase = waves[ch][i + 1].phase;
  }
  return 1;
}

bool Vibrosonics::modifySinWave(int idx, int ch, int freq, int amp, int phase) {
  if (!isValidIndex(idx, ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT MODIFY WAVE");
    #endif
    return 0;
  }

  if (freq < 0 || phase < 0) {
    #ifdef DEBUG
    Serial.println("CANNOT MODIFY WAVE, FREQ AND PHASE MUST BE POSTIVE!");
    #endif
    return 0;
  } 

  waves[ch][idx].amp = amp;
  waves[ch][idx].freq = freq;
  waves[ch][idx].phase = phase;
  return 1;
}

void Vibrosonics::resetSinWaves(int ch) {
  if (!isValidChannel(ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT RESET WAVES");
    #endif
    return;
  }
  // restore amplitudes and frequencies on ch
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    waves[ch][i].amp = 0;
    waves[ch][i].freq = 0;
    waves[ch][i].phase = 0;
  }
  num_waves[ch] = 0;
}

int Vibrosonics::getNumWaves(int ch) { 
  if (!isValidChannel(ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT GET NUM WAVES");
    #endif
    return -1;
  }
  return num_waves[ch];
}

int Vibrosonics::getFreq(int idx, int ch) {
  if (!isValidIndex(idx, ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT GET FREQUENCY");
    #endif
    return -1;
  }
  return waves[ch][idx].freq;
}

int Vibrosonics::getAmp(int idx, int ch) { 
  if (!isValidIndex(idx, ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT GET AMPLITUDE");
    #endif
    return -1;
  }
  return waves[ch][idx].amp;
}

int Vibrosonics::getPhase(int idx, int ch) {
  if (!isValidIndex(idx, ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT GET PHASE");
    #endif
    return -1;
  }
  return waves[ch][idx].phase;
}

bool Vibrosonics::setFreq(int freq, int idx, int ch) {
  if (!isValidIndex(idx, ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT SET FREQUENCY");
    #endif
    return 0;
  } 
  waves[ch][idx].phase = freq;
  return 1;
}

bool Vibrosonics::setAmp(int amp, int idx, int ch) {
  if (!isValidIndex(idx, ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT SET AMPLITUDE");
    #endif
    return 0;
  } 
  waves[ch][idx].phase = amp;
  return 1;
}

bool Vibrosonics::setPhase(int phase, int idx, int ch) {
  if (!isValidIndex(idx, ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT SET PHASE");
    #endif
    return 0;
  } 
  waves[ch][idx].phase = phase;
  return 1;
}

bool Vibrosonics::isValidChannel(int ch) {
  if (ch < 0 || ch > AUD_OUT_CH - 1) {
    #ifdef DEBUG
    Serial.printf("CHANNEL %d IS INVALID! ", ch);
    #endif
    return 0;
  }
  return 1;
}

bool Vibrosonics::isValidIndex(int idx, int ch) {
  if (!isValidChannel(ch)) return 0;
  if (idx < 0 || idx >= num_waves[ch]) {
    #ifdef DEBUG
    Serial.printf("INDEX %d IS INVALID! ", ch);
    #endif
    return 0;
  }
  return 1;
}

void Vibrosonics::printSinWaves(void) {
  for (int c = 0; c < AUD_OUT_CH; c++) {
    Serial.printf("CH %d (F, A, P?): ", c);
    for (int i = 0; i < num_waves[c]; i++) {
    Serial.printf("(%03d, %03d", waves[c][i].amp, waves[c][i].freq);
    if (waves[c][i].phase > 0) {
      Serial.printf(", %04d", waves[c][i].phase);
    }
    Serial.print(") ");
    }
    Serial.println();
  }
}

float Vibrosonics::get_sin_wave_val(Wave w) {
  float sin_wave_freq_idx = (sin_wave_idx * w.freq + w.phase) * _SAMPLING_FREQ;
  int sin_wave_position = (sin_wave_freq_idx - floor(sin_wave_freq_idx)) * SAMPLING_FREQ;
  return w.amp * sin_wave[sin_wave_position];
}

float Vibrosonics::get_sum_of_channel(int ch) {
  float sum = 0.0;
  for (int s = 0; s < num_waves[ch]; s++) {
    if (waves[ch][s].amp == 0 || (waves[ch][s].freq == 0 && waves[ch][s].phase == 0)) continue;
    sum += get_sin_wave_val(waves[ch][s]);
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
  for (int i = 0; i < AUD_OUT_BUFFER_SIZE; i++) {
    // sum together the sine waves for left channel and right channel
    for (int c = 0; c < AUD_OUT_CH; c++) {
      // add windowed value to the existing values in scratch pad audio output buffer at this moment in time
      generateAudioBuffer[c][generateAudioIdx] += get_sum_of_channel(c) * cos_wave_w[i];
    }

    // copy final, synthesized values to volatile audio output buffer
    if (i < AUD_IN_BUFFER_SIZE) {
    // shifting output by 128.0 for ESP32 DAC, min max ensures the value stays between 0 - 255 to ensure clipping won't occur
      for (int c = 0; c < AUD_OUT_CH; c++) {
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
    for (int c = 0; c < AUD_OUT_CH; c++) {
      generateAudioBuffer[c][generateAudioIdxCpy] = 0.0;
    }
    generateAudioIdxCpy += 1;
    if (generateAudioIdxCpy == GEN_AUD_BUFFER_SIZE) generateAudioIdxCpy = 0;
  }
  // determine the next position in the sine wave table, and scratch pad audio output buffer to counter phase cosine wave
  generateAudioIdx = int(generateAudioIdx - FFT_WINDOW_SIZE + GEN_AUD_BUFFER_SIZE) % int(GEN_AUD_BUFFER_SIZE);
  sin_wave_idx = int(sin_wave_idx - FFT_WINDOW_SIZE + SAMPLING_FREQ) % int(SAMPLING_FREQ);

  #ifdef DEBUG
  printSinWaves();
  #endif
}

/*/
########################################################
Functions related to sampling and outputting audio by interrupt
########################################################
/*/

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
  #if AUD_OUT_CH == 2
  dacWrite(AUD_OUT_PIN_R, AUD_OUT_BUFFER[1][AUD_OUT_IDX]);
  #endif

  #ifdef USE_FFT
  AUD_IN_BUFFER[AUD_IN_BUFFER_IDX] = adc1_get_raw(AUD_IN_PIN);
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
    for (int c = 0; c < AUD_OUT_CH; c++) {
    generateAudioBuffer[c][i] = 0.0;
      if (i < AUD_OUT_BUFFER_SIZE) {
        AUD_OUT_BUFFER[c][i] = 128;
      }
    }
  }

  // reset waves
  for (int i = 0; i < AUD_OUT_CH; i++) {
    resetSinWaves(i);
  }
}