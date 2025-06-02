#ifndef VIBROSONICS_H
#define VIBROSONICS_H

#include <Arduino.h>
#include <SPI.h>
#include <Math.h>
#include "AD56X4_vibrosonics.h"
#include "arduinoFFT.h"

#define SS_PIN 33 // Slave select pin; I chose Pin 33 on the ESP32 Feather, but others could be used so long as it is updated here.

#define AD56X4_SIZE 14 // Change to 12 or 16 if using the AD5624 or AD5664.
#define SAMPLE_RATE 8192 // 8kHz sample rate. This leaves us with 122 μs of time between interrupts.
#define FFT_SAMPLES 512 // FFT window size. The higher the window, the better the frequency resolution but the greater the delay in response to frequency changes.
#define NOISE_THRESHOLD 1100.0 // Determines the threshold under which signals with lower amplitudes should be disregarded as noise.
#define FFT_TIMEOUT 500 // The time, in miliseconds, after which output should stop if no signals have surpassed the threshold.

#define MIDS_INPUT_MIN 250.0 // Mids range of 250 - 2000 Hz translated into haptic range of 80 - 200Hz
#define MIDS_INPUT_MAX 2000.0
#define MIDS_OUTPUT_MIN 80.0
#define MIDS_OUTPUT_MAX 200.0

#define HIGHS_INPUT_MIN 2000.0 // Highs range of 2000 - 20,000 Hz translated into haptic range of 200 - 400 Hz
#define HIGHS_INPUT_MAX 20000.0
#define HIGHS_OUTPUT_MIN 200.0
#define HIGHS_OUTPUT_MAX 400.0

extern AD56X4Class dac; // DAC object
extern ArduinoFFT<double> FFT; // FFT object
extern byte channel[]; // Array for holding bytes corresponding to our AD564X4's A, B, C, and D channels.
extern hw_timer_t *SAMPLING_TIMER; // Interrupt timer
extern uint16_t midsWave[]; // Stores current mids wave sinusoid data
extern uint16_t highsWave[]; // Stores current highs wave sinusoid data
extern double samples[]; // Array where we collect samples
extern double vReal[]; // Holds "real" component of samples for analysis
extern double vImag[]; // Holds "imaginary" component of samples for analysis
extern volatile int midsFrequency; // Output of mids frequency analysis
extern volatile int highsFrequency; // Output of highs frequency analysis
extern volatile int FFTtimer; // Keeps track of time since last new frequency for timeout purposes.
extern volatile int iterator; // Used in the interupt to collect FFT_SAMPLES amount of samples before passing on that window for analysis and resetting to zero.
extern volatile int wave_iterator; // Used when outputting waves to iterate through their sinusoid arrays
extern volatile int current_mids; // Used to store current mids frequency so it can be compared against new ones
extern volatile int current_highs; // Used to store current hgihs frequency so it can be compared against new ones
extern volatile bool updateWave; // This is made true when the interrupt collects a 512 sample window, triggering FFT analysis.

void initialize(void (*userFunc)()); // Initialization of Serial and SPI communication, the AD56X4 DAC, analog input, and the interrupt.
void generateWave(int, uint16_t[], double); // Generates sinusoid at a specified frequency, in a given array, and at a specified volume
void analyzeWave(); // Performs FFT analysis on mids and highs sample windows
void obtain_raw_analog(); // Useful for identifying where we should map inputs in speakerMode()
double mapFrequency(double, double, double, double, double); // Maps analyzed frequencies down into the haptic range
int getBin(double); // Estimates the FFT bin of a frequency based on the window size and sample rate
void speakerMode(byte); // Interrupt function for Speaker Mode: simply reads the input signal and outputs it directly to one of our analog output channels
void FFTMode(); // Interrupt function for FFT Mode: performs FFT analysis on the input signal to generate mids and highs haptic frequencies that are outputted to all four of our analog output channels
void FFTMode_loop(double); // Loop function for FFT Mode: triggers analysis when new sample windows are generated, generates sinusoid data for new frequencies

#endif