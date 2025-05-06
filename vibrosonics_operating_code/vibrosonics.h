#ifndef VIBROSONICS_H
#define VIBROSONICS_H

#include <Arduino.h>
#include <SPI.h>
#include <Math.h>
#include "AD56X4_alt.h"
#include "arduinoFFT.h"

#define SAMPLE_RATE 8192
#define SS_PIN 33
#define FFT_SAMPLES 512
#define NOISE_THRESHOLD 11 00.0

#define MIDS_INPUT_MIN 250.0
#define MIDS_INPUT_MAX 2000.0
#define MIDS_OUTPUT_MIN 80.0
#define MIDS_OUTPUT_MAX 200.0

#define HIGHS_INPUT_MIN 2000.0
#define HIGHS_INPUT_MAX 20000.0
#define HIGHS_OUTPUT_MIN 200.0
#define HIGHS_OUTPUT_MAX 400.0

extern AD56X4Class dac;
extern byte channel[];
extern hw_timer_t *SAMPLING_TIMER;
extern uint16_t midsWave[];
extern uint16_t highsWave[];
extern double samples[];
extern double vReal[];
extern double vImag[];
extern volatile int midsFrequency;
extern volatile int highsFrequency;
extern volatile int FFTtimer;
extern volatile int iterator;
extern volatile int wave_iterator;
extern volatile int current_mids;
extern volatile int current_highs;
extern volatile bool updateWave;

extern ArduinoFFT<double> FFT;

void initialize(void (*userFunc)());
void generateWave(int, uint16_t[SAMPLE_RATE], double);
void analyzeWave();
void obtain_raw_analog();
double mapFrequency(double, double, double, double, double);
int getBin(double);
void speakerMode(byte);
void FFTMode();
void FFTMode_loop(double);

#endif