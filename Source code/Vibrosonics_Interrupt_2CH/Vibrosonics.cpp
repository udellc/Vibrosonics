#include "Vibrosonics.h"

Vibrosonics::Vibrosonics() {

}

// the procedure that is called when timer interrupt is triggered
void Vibrosonics::ON_SAMPLING_TIMER() {
  AUD_IN_OUT();
}

void Vibrosonics::init(void) {
    analogReadResolution(12);
    delay(3000);
    Serial.println("Ready");

    // initialize generate audio and output buffers to 0, not necassary but helps prevent glitches when program boots
    for (int i = 0; i < GEN_AUD_BUFFER_SIZE; i++) {
        for (int c = 0; c < AUD_OUT_CH; c++) {
        generateAudioBuffer[c][i] = 0.0;
            if (i < AUD_OUT_BUFFER_SIZE) {
                AUD_OUT_BUFFER[c][i] = 128;
            }
        }
    }

    // calculate values of cosine and sine wave at certain sampling frequency
    calculateWindowingWave();
    calculateWaves();
    for (int i = 0; i < AUD_OUT_CH; i++) {
      resetSinWaves(i);
    }

    // setup pins
    pinMode(AUD_OUT_PIN_L, OUTPUT);
    pinMode(AUD_OUT_PIN_R, OUTPUT);

    delay(1000);

    // setup timer interrupt for audio sampling
    SAMPLING_TIMER = timerBegin(0, 80, true);                       // setting clock prescaler 1MHz (80MHz / 80)
    timerAttachInterrupt(SAMPLING_TIMER, &ON_SAMPLING_TIMER, true); // attach interrupt function
    timerAlarmWrite(SAMPLING_TIMER, sampleDelayTime, true);         // trigger interrupt every @sampleDelayTime microseconds
    timerAlarmEnable(SAMPLING_TIMER);                               // enabled interrupt

    timerAlarmDisable(SAMPLING_TIMER);
    Serial.println("Setup complete");
    timerAlarmEnable(SAMPLING_TIMER);
}

bool Vibrosonics::ready(void) {
    if (!AUD_IN_BUFFER_FULL()) return 0;
    #ifndef USE_FFT
    RESET_AUD_IN_OUT_IDX();
    #endif
    return 1;
}

void Vibrosonics::pullSamples(void) {
    // in debug mode, print total time spent in loop, optionally interrupt timer can be disabled
    #ifdef DEBUG
    // timerAlarmDisable(SAMPLING_TIMER); // optionally disable interrupt timer during debug mode
    loop_time = micros();
    #endif

    #ifdef USE_FFT
    for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
        vReal[i] = AUD_IN_BUFFER[i];
        vImag[i] = 0.0;
    }
    #endif
    // reset AUD_IN_BUFFER_IDX to 0, so interrupt can continue to perform audio input and output
    RESET_AUD_IN_OUT_IDX();
}

void Vibrosonics::performFFT(void) {
    for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
        vImag[i] = 0.0;
    }
    // use arduinoFFT
    FFT.DCRemoval();                                  // DC Removal to reduce noise
    FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
    FFT.Compute(FFT_FORWARD);                         // Compute FFT
    FFT.ComplexToMagnitude();                         // Compute frequency magnitudes
}

// FFT data processing
void Vibrosonics::processData(void) {  
    // copy values calculated by FFT to freqs
    storeFreqs();

    // noise flooring based on a threshold
    noiseFloor(freqs, 20.0);

    // finds peaks in data, stores index of peak in FFTPeaks and Amplitude of peak in FFTPeaksAmp
    findMajorPeaks(freqs);

    // assign sine waves based on data found by major peaks
    resetSinWaves(0);
    // resetSinWaves(1);
    assignSinWaves(FFTPeaks, FFTPeaksAmp, FFT_WINDOW_SIZE_BY2 >> 1);
    mapAmplitudes();
}

/*/
########################################################
Functions relating to assigning sine waves and mapping their frequencies and amplitudes
########################################################
/*/

// assigns the frequencies and amplitudes found by majorPeaks to sine waves
void Vibrosonics::assignSinWaves(int* freqData, float* ampData, int size) {  
    // assign sin_waves and freq/amps that are above 0, otherwise skip
    for (int i = 0; i < size; i++) {
        // skip storing if ampData is 0, or freqData is 0
        if (ampData[i] == 0.0 || freqData[i] == 0) continue;
        // assign frequencies below bass to left channel, otherwise to right channel
        if (freqData[i] <= bassIdx) {
            int freq = freqData[i];
            // if the difference of energy around the peak is greater than threshold
            if (abs(freqs[freqData[i] - 1] - freqs[freqData[i] + 1]) > 100) {
                // assign frequency based on whichever side is greater
                freq = freqs[freqData[i] - 1] > freqs[freqData[i] + 1] ? (freqData[i] - 0.5) : (freqData[i] + 0.5);
            }
            addSinWave(freq * freqRes, ampData[i], 0, 0);
        } else {
            int interpFreq = interpolateAroundPeak(freqs, freqData[i]);
            #if AUD_OUT_CH == 2
            addSinWave(interpFreq, ampData[i], 1, 0);
            #else
            addSinWave(interpFreq, ampData[i], 0, 0);
            #endif
        }
    }
}

// maps amplitudes between 0 and 127 range to correspond to 8-bit (0, 255) DAC on ESP32 Feather mapping is based on the MAX_AMP_SUM
void Vibrosonics::mapAmplitudes(void) {
    // map amplitudes on both channels
    for (int c = 0; c < AUD_OUT_CH; c++) {
        int ampSum = 0;
        for (int i = 0; i < num_waves[c]; i++) {
            int amplitude = waves[c][i].amp;
            if (amplitude == 0) continue;
            ampSum += amplitude;
        }
        // since all amplitudes are 0, then there is no need to map between 0-127 range.
        if (ampSum == 0) { 
        num_waves[c] = 0;
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
        if (ampSum == 0) num_waves[c] = 0;
    }
}

/*/
########################################################
  Functions related to FFT analysis
########################################################
/*/

// store the current window into freqs
void Vibrosonics::storeFreqs (void) {
    for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
        freqs[i] = vReal[i] * freqWidth;
    }
}

// noise flooring based on a set threshold
void Vibrosonics::noiseFloor(float *data, float threshold) {
    for (int i = 0; i < FFT_WINDOW_SIZE_BY2; i++) {
        if (data[i] < threshold) {
            data[i] = 0.0;
        }
    }
}

// finds all the peaks in the fft data* and removes the minimum peaks to contain output to @MAX_NUM_PEAKS
void Vibrosonics::findMajorPeaks(float* data) {
    // restore output arrays
    for (int i = 0; i < FFT_WINDOW_SIZE_BY2 >> 1; i++) {
        FFTPeaks[i] = 0;
        FFTPeaksAmp[i] = 0;
    }
    // total sum of data
    int peaksFound = 0;
    float maxPeak = 0;
    int maxPeakIdx = 0;
    // iterate through data to find peaks
    for (int f = 1; f < FFT_WINDOW_SIZE_BY2 - 1; f++) {
        // determines if data[f] is a peak by comparing with previous and next location, otherwise continue
        if ((data[f - 1] < data[f]) && (data[f] > data[f + 1])) {
        float peakSum = data[f - 1] + data[f] + data[f + 1];
        if (peakSum > maxPeak) {
            maxPeak = peakSum;
            maxPeakIdx = f;
        }
        // store sum around the peak and index of peak
        FFTPeaksAmp[peaksFound] = peakSum;
        FFTPeaks[peaksFound++] = f;
        }
    }
    // if needed, remove a certain number of the minumum peaks to contain output to @maxNumberOfPeaks
    int numPeaksToRemove = peaksFound - MAX_NUM_PEAKS;
    for (int j = 0; j < numPeaksToRemove; j++) {
        // store minimum as the maximum peak
        float minimumPeak = maxPeak;
        int minimumPeakIdx = maxPeakIdx;
        // find the minimum peak and replace with zero
        for (int i = 0; i < peaksFound; i++) {
            float thisPeakAmplitude = FFTPeaksAmp[i];
            if (thisPeakAmplitude > 0 && thisPeakAmplitude < minimumPeak) {
                minimumPeak = thisPeakAmplitude;
                minimumPeakIdx = i;
            }
        }
        FFTPeaks[minimumPeakIdx] = 0;
        FFTPeaksAmp[minimumPeakIdx] = 0;
    }
}

// interpolation based on the weight of amplitudes around a peak
int Vibrosonics::interpolateAroundPeak(float *data, int indexOfPeak) {
    float prePeak = indexOfPeak == 0 ? 0.0 : data[indexOfPeak - 1];
    float atPeak = data[indexOfPeak];
    float postPeak = indexOfPeak == FFT_WINDOW_SIZE_BY2 ? 0.0 : data[indexOfPeak + 1];
    // summing around the index of maximum amplitude to normalize magnitudeOfChange
    float peakSum = prePeak + atPeak + postPeak;
    // interpolating the direction and magnitude of change, and normalizing from -1.0 to 1.0
    float magnitudeOfChange = ((atPeak + postPeak) - (atPeak + prePeak)) / (peakSum > 0.0 ? peakSum : 1.0);
    
    // return interpolated frequency
    return int(round((float(indexOfPeak) + magnitudeOfChange) * freqRes));
}

/*/
########################################################
  Functions related to generating values for the circular audio output buffer
########################################################
/*/

// calculate values for cosine function that is used for smoothing transition between frequencies and amplitudes (0.5 * (1 - cos((2PI / T) * x)), where T = AUD_OUT_BUFFER_SIZE
void Vibrosonics::calculateWindowingWave(void) {
    float resolution = float(2.0 * PI / AUD_OUT_BUFFER_SIZE);
    for (int i = 0; i < AUD_OUT_BUFFER_SIZE; i++) {
        cos_wave_w[i] = 0.5 * (1.0 - cos(float(resolution * i)));
    }
}

// calculate values for 1Hz sine wave @SAMPLING_FREQ sample rate
void Vibrosonics::calculateWaves(void) {
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

// assigns a sine wave to specified channel, phase makes most sense to be between 0 and SAMPLING_FREQ where if phase == SAMPLING_FREQ / 2 then the wave is synthesized in counter phase
void Vibrosonics::addSinWave(int freq, int amp, int ch, int phase) {
    if (freq < 0 || phase < 0) {
        #ifdef DEBUG
        Serial.printf("CANNOT ADD SINE WAVE, FREQ AND PHASE MUST BE POSTIVE!");
        #endif
        return;
    } 
    if (ch > AUD_OUT_CH - 1 || ch < 0) {
        #ifdef DEBUG
        Serial.printf("CANNOT ADD SINE WAVE, CHANNEL %d ISN'T DEFINED\n", ch);
        #endif
        return;
    }

    // ensures that the sum of waves in the channels is no greater than MAX_NUM_WAVES
    int num_waves_count = 0;
    for (int c = 0; c < AUD_OUT_CH; c++) {
        num_waves_count += num_waves[c];
    }

    if (num_waves_count == MAX_NUM_WAVES) {
        #ifdef DEBUG
        Serial.println("CANNOT ASSIGN MORE WAVES, CHANGE MAX_NUM_WAVES!");
        #endif
        return;
    }

    
    waves[ch][num_waves[ch]].amp = amp;
    waves[ch][num_waves[ch]].freq = freq;
    waves[ch][num_waves[ch]].phase = phase;
    num_waves[ch] += 1;
}

// removes a sine wave at specified index and channel
void Vibrosonics::removeSinWave(int idx, int ch) {
    if (ch > AUD_OUT_CH - 1 || ch < 0) {
        #ifdef DEBUG
        Serial.printf("CANNOT REMOVE WAVE, CHANNEL %d ISN'T DEFINED!\n", ch);
        #endif
        return;
    }
    if (idx >= num_waves[ch]) {
        #ifdef DEBUG
        Serial.printf("CANNOT REMOVE WAVE, INDEX %d DOESN'T EXIST!\n", idx);
        #endif
        return;
    }

    for (int i = idx; i < --num_waves[ch]; i++) {
        waves[ch][i].amp = waves[ch][i + 1].amp;
        waves[ch][i].freq = waves[ch][i + 1].freq;
        waves[ch][i].phase = waves[ch][i + 1].phase;
    }
}

// modify sine wave at specified index and channel to desired frequency and amplitude
void Vibrosonics::modifySinWave(int idx, int ch, int freq, int amp, int phase) {
    if (freq < 0 || phase < 0) {
        #ifdef DEBUG
        Serial.printf("CANNOT ADD SINE WAVE, FREQ AND PHASE MUST BE POSTIVE!");
        #endif
        return;
    } 
    if (ch > AUD_OUT_CH - 1) {
        #ifdef DEBUG
        Serial.printf("CANNOT MODIFY WAVE, CHANNEL %d ISN'T DEFINED!\n", ch);
        #endif
        return;
    }
    if (idx >= num_waves[ch]) {
        #ifdef DEBUG
        Serial.printf("CANNOT MODIFY WAVE, INDEX %d DOESN'T EXIST!\n", idx);
        #endif
        return;
    }
    waves[ch][idx].amp = amp;
    waves[ch][idx].freq = freq;
    waves[ch][idx].phase = phase;
}

// sets all sine waves and frequencies to 0 on specified channel
void Vibrosonics::resetSinWaves(int ch) {
    if (ch > AUD_OUT_CH - 1) {
        #ifdef DEBUG
        Serial.printf("CANNOT RESET WAVES, CHANNEL %d ISN'T DEFINED!\n", ch);
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

// prints assigned sine waves
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

// returns value of sine wave at given frequency and amplitude
float Vibrosonics::get_sin_wave_val(Wave w) {
    float sin_wave_freq_idx = (sin_wave_idx * w.freq + w.phase) * _SAMPLING_FREQ;
    int sin_wave_position = (sin_wave_freq_idx - floor(sin_wave_freq_idx)) * SAMPLING_FREQ;
    return w.amp * sin_wave[sin_wave_position];
}

// returns sum of sine waves of given channel
float Vibrosonics::get_sum_of_channel(int ch) {
    float sum = 0.0;
    for (int s = 0; s < num_waves[ch]; s++) {
        if (waves[ch][s].amp == 0 || (waves[ch][s].freq == 0 && waves[ch][s].phase == 0)) continue;
        sum += get_sin_wave_val(waves[ch][s]);
    }
    return sum;
}

// generates values for one window of audio output buffer
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
    Serial.print("time spent processing in microseconds: ");
    Serial.println(micros() - loop_time);
    printSinWaves();
    // timerAlarmEnable(SAMPLING_TIMER);  // enable interrupt timer
    #endif
}

/*/
########################################################
Functions related to sampling and outputting audio by interrupt
########################################################
/*/

// returns true if audio input buffer is full
bool Vibrosonics::AUD_IN_BUFFER_FULL(void) {
    return !(AUD_IN_BUFFER_IDX < AUD_IN_BUFFER_SIZE);
}

// restores AUD_IN_BUFFER_IDX, and ensures AUD_OUT_BUFFER is synchronized
void Vibrosonics::RESET_AUD_IN_OUT_IDX(void) {
    AUD_OUT_BUFFER_IDX += AUD_IN_BUFFER_SIZE;
    if (AUD_OUT_BUFFER_IDX >= AUD_OUT_BUFFER_SIZE) AUD_OUT_BUFFER_IDX = 0;
    AUD_IN_BUFFER_IDX = 0;
}

// outputs sample from AUD_OUT_BUFFER to DAC and reads sample from ADC to AUD_IN_BUFFER
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