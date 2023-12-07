#include "Vibrosonics.h"

bool Vibrosonics::isValidId(int id) {
  if (id >= 0 && id < MAX_NUM_WAVES) {
    if (waves_map[id].valid) return 1;
  }
  #ifdef DEBUG
  Serial.printf("WAVE ID %d INVALID! ", id);
  #endif
  return 0;
}

bool Vibrosonics::isValidChannel(int ch) {
  if (ch < 0 || ch > NUM_OUT_CH - 1) {
    #ifdef DEBUG
    Serial.printf("CHANNEL %d IS INVALID! ", ch);
    #endif
    return 0;
  }
  return 1;
}

int Vibrosonics::addWave(int ch, int freq, int amp, int phase) {
  if (!isValidChannel(ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT ADD WAVE");
    #endif
    return -1;
  }

  if (freq < 0 || phase < 0) {
    #ifdef DEBUG
    Serial.println("CANNOT ADD WAVE, FREQ AND PHASE MUST BE POSTIVE!");
    #endif
    return -1;
  } 

  int unmapped_idx = -1;
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    if (waves_map[i].valid == 0) {
      unmapped_idx = i;
      break;
    }
  }

  if (unmapped_idx == -1) {
    #ifdef DEBUG
    Serial.println("CANNOT ADD WAVE, CHANGE MAX_NUM_WAVES!");
    #endif
    return unmapped_idx;
  }
  waves_map[unmapped_idx].valid = 1;
  waves_map[unmapped_idx] = wave_map(ch, freq, amp, phase);
  return unmapped_idx;
}

void Vibrosonics::removeWave(int id) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT REMOVE WAVE");
    #endif
    return;
  }
  waves_map[id].valid = 0;
  waves_map[id].w = wave();
}

void Vibrosonics::modifyWave(int id, int freq, int amp, int phase, int ch) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT MODIFY WAVE");
    #endif
    return;
  }

  if (freq < 0 || phase < 0) {
    #ifdef DEBUG
    Serial.println("CANNOT MODIFY WAVE, FREQ AND PHASE MUST BE POSTIVE!");
    #endif
    return;
  } 

  waves_map[id].ch = ch;
  waves_map[id].w = wave(freq, amp, phase);
}

void Vibrosonics::resetWaves(int ch) {
  if (!isValidChannel(ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT RESET WAVES");
    #endif
    return;
  }

  // restore amplitudes and frequencies on ch, invalidate wave
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    if (waves_map[i].ch != ch) continue;
    if (waves_map[i].valid == 0) continue;
    waves_map[i].valid = 0;
    waves_map[i].w = wave();
  }
}

void Vibrosonics::resetAllWaves(void) {
  // restore amplitudes and frequencies on ch, invalidate wave
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    if (waves_map[i].valid == 0) continue;
    waves_map[i].valid = 0;
    waves_map[i].w = wave();
  }
}

int Vibrosonics::getNumWaves(int ch) { 
  if (!isValidChannel(ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT GET NUM WAVES");
    #endif
    return -1;
  }
  // restore amplitudes and frequencies on ch, invalidate wave
  int count = 0;
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    if (waves_map[i].valid == 0) continue;
    if (waves_map[i].ch != ch) continue;
    count += 1;
  }
  return count;
}

int Vibrosonics::getFreq(int id) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT GET FREQUENCY");
    #endif
    return -1;
  }

  return waves_map[id].w.freq;
}

int Vibrosonics::getAmp(int id) { 
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT GET AMPLITUDE");
    #endif
    return -1;
  }

  return waves_map[id].w.freq;
}

int Vibrosonics::getPhase(int id) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT GET PHASE");
    #endif
    return -1;
  }

  return waves_map[id].w.phase;
}

bool Vibrosonics::setFreq(int id, int freq) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT SET FREQUENCY");
    #endif
    return 0;
  } 

  waves_map[id].w.freq = freq;
  return 1;
}

bool Vibrosonics::setAmp(int id, int amp) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT SET AMPLITUDE");
    #endif
    return 0;
  } 

  waves_map[id].w.freq = amp;
  return 1;
}

bool Vibrosonics::setPhase(int id, int phase) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT SET PHASE");
    #endif
    return 0;
  } 

  waves_map[id].w.freq = phase;
  return 1;
}

void Vibrosonics::printWaves(void) {
  for (int c = 0; c < NUM_OUT_CH; c++) {
    Serial.printf("CH %d (F, A, P?): ", c);
    for (int i = 0; i < MAX_NUM_WAVES; i++) {
      if (waves_map[i].valid == 0) continue;
      if (waves_map[i].ch != c) continue;
      Serial.printf("(%03d, %03d", waves_map[i].w.freq, waves_map[i].w.amp);
      if (waves_map[i].w.phase > 0) {
        Serial.printf(", %04d", waves_map[i].w.phase);
      }
      Serial.print(") ");
    }
    Serial.println();
  }
}