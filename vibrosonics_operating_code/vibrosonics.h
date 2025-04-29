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
#define NOISE_THRESHOLD 1000.0

extern AD56X4Class dac;
extern byte channel[];
extern hw_timer_t *SAMPLING_TIMER;
extern uint16_t sinusoid[];
extern double samples[];
extern double vReal[];
extern double vImag[];
extern volatile int frequency;
extern volatile int FFTtimer;
extern volatile int iterator;
extern volatile int current_wave;
extern volatile bool updateWave;

extern ArduinoFFT<double> FFT;

void initialize(void (*userFunc)());
void generateWave(int, uint16_t[SAMPLE_RATE], double);
void analyzeWave();
void obtain_raw_analog();
double mapFrequency(double);
void speakerMode(byte);
void FFTMode();
void FFTMode_loop(double);

#endif