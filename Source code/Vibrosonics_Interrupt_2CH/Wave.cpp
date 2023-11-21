#include "Vibrosonics.h"

int Vibrosonics::mapWave(int ch, int idx) {
  int unmapped_idx = 0;
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    if (waves_map[i].valid == 0) {
      unmapped_idx = i;
      break;
    }
  }
  waves_map[unmapped_idx].ch = ch;
  waves_map[unmapped_idx].idx = idx;
  waves_map[unmapped_idx].valid = 1;
  return unmapped_idx;
}

void Vibrosonics::unmapWave(int id) {
  waves_map[id].valid = 0;
}

void Vibrosonics::remapWave(int id, int ch, int idx) {
  waves_map[id].ch = ch;
  waves_map[id].idx = idx;
}

int Vibrosonics::getWaveId(int ch, int idx) {
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    if (waves_map[i].valid == 0) continue;
    if (waves_map[i].ch != ch) continue;
    if (waves_map[i].idx == idx) return i;
  }
  return -1;
}

int Vibrosonics::getWaveCh(int id) {
  return waves_map[id].ch;
}

int Vibrosonics::getWaveIdx(int id) {
  return waves_map[id].idx;
}

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

  // ensures that the sum of waves in the channels is no greater than MAX_NUM_WAVES
  int num_waves_count = 0;
  for (int c = 0; c < NUM_OUT_CH; c++) {
    num_waves_count += num_waves[c];
  }

  if (num_waves_count == MAX_NUM_WAVES) {
    #ifdef DEBUG
    Serial.println("CANNOT ADD WAVE, CHANGE MAX_NUM_WAVES!");
    #endif
    return -1;
  }

  int idx = num_waves[ch];
  waves[ch][idx].amp = amp;
  waves[ch][idx].freq = freq;
  waves[ch][idx].phase = phase;
  num_waves[ch] += 1;
  int id = mapWave(ch, idx);
  return id;
}

void Vibrosonics::removeWave(int id) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT REMOVE WAVE");
    #endif
    return;
  }

  int ch = getWaveCh(id);
  int idx = getWaveIdx(id);

  unmapWave(id);
  num_waves[ch] -= 1;

  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    if (i < num_waves[ch]) {
      waves[ch][i].amp = waves[ch][i + 1].amp;
      waves[ch][i].freq = waves[ch][i + 1].freq;
      waves[ch][i].phase = waves[ch][i + 1].phase;
      remapWave(getWaveId(ch, i + 1), ch, i);
    }
    else {
      resetWave(ch, i);
    }
  }
  // for (int i = idx; i < num_waves[ch]; i++) {
  //   waves[ch][i].amp = waves[ch][i + 1].amp;
  //   waves[ch][i].freq = waves[ch][i + 1].freq;
  //   waves[ch][i].phase = waves[ch][i + 1].phase;
  //   remapWave(getWaveId(ch, i + 1), ch, i);
  // }
}

void Vibrosonics::modifyWave(int id, int freq, int amp, int phase) {
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

  int ch = getWaveCh(id);
  int idx = getWaveIdx(id);

  waves[ch][idx].amp = amp;
  waves[ch][idx].freq = freq;
  waves[ch][idx].phase = phase;
}

void Vibrosonics::resetWave(int ch, int idx) {
  waves[ch][idx].amp = 0;
  waves[ch][idx].freq = 0;
  waves[ch][idx].phase = 0;
}

void Vibrosonics::resetWaves(int ch) {
  if (!isValidChannel(ch)) {
    #ifdef DEBUG
    Serial.println("CANNOT RESET WAVES");
    #endif
    return;
  }

  // restore amplitudes and frequencies on ch
  for (int i = 0; i < MAX_NUM_WAVES; i++) {
    resetWave(ch, i);
    if (waves_map[i].valid == 0) continue;
    if (getWaveCh(i) == ch) {
      unmapWave(i);
    }
  }
  num_waves[ch] = 0;
}

void Vibrosonics::resetAllWaves(void) {
  for (int ch = 0; ch < NUM_OUT_CH; ch++) {
    resetWaves(ch);
  }
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

int Vibrosonics::getFreq(int id) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT GET FREQUENCY");
    #endif
    return -1;
  }

  int ch = getWaveCh(id);
  int idx = getWaveIdx(id);

  return waves[ch][idx].freq;
}

int Vibrosonics::getAmp(int id) { 
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT GET AMPLITUDE");
    #endif
    return -1;
  }

  int ch = getWaveCh(id);
  int idx = getWaveIdx(id);

  return waves[ch][idx].amp;
}

int Vibrosonics::getPhase(int id) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT GET PHASE");
    #endif
    return -1;
  }

  int ch = getWaveCh(id);
  int idx = getWaveIdx(id);

  return waves[ch][idx].phase;
}

bool Vibrosonics::setFreq(int id, int freq) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT SET FREQUENCY");
    #endif
    return 0;
  } 

  int ch = getWaveCh(id);
  int idx = getWaveIdx(id);

  waves[ch][idx].freq = freq;
  return 1;
}

bool Vibrosonics::setAmp(int id, int amp) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT SET AMPLITUDE");
    #endif
    return 0;
  } 

  int ch = getWaveCh(id);
  int idx = getWaveIdx(id);

  waves[ch][idx].amp = amp;
  return 1;
}

bool Vibrosonics::setPhase(int id, int phase) {
  if (!isValidId(id)) {
    #ifdef DEBUG
    Serial.println("CANNOT SET PHASE");
    #endif
    return 0;
  } 

  int ch = getWaveCh(id);
  int idx = getWaveIdx(id);

  waves[ch][idx].phase = phase;
  return 1;
}

void Vibrosonics::printWaves(void) {
  for (int c = 0; c < NUM_OUT_CH; c++) {
    Serial.printf("CH %d (F, A, P?): ", c);
    for (int i = 0; i < num_waves[c]; i++) {
    Serial.printf("(%03d, %03d", waves[c][i].freq, waves[c][i].amp);
    if (waves[c][i].phase > 0) {
      Serial.printf(", %04d", waves[c][i].phase);
    }
    Serial.print(") ");
    }
    Serial.println();
  }
}