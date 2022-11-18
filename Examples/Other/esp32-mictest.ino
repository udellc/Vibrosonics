#include <arduinoFFT.h>
#include <Arduino.h>

#define micInPin A0
#define ledPin A5

// audio input settings
const int SampleFreq = 8192;
const int RecordTime = 4;
const int FFTWinSize = 1024;
const float correlationthresh = 0.5;

// timer interrupt

const int SerialPlotterLow = 0;
const int SerialPlotterHigh = 3300;
// 12 bit ADC to voltage conversion
const float refV = 3300.0/4095.0;

// number of windows in recorded frequency
const int FreqWinCount = RecordTime * SampleFreq / FFTWinSize;

void setup() {
  // ESP32 baud rate is 115200
  Serial.begin(115200);

  pinMode(micInPin, INPUT);
  pinMode(ledPin, OUTPUT);

  digitalWrite(ledPin, LOW);
}

void loop() {

  int adcSample = analogRead(micInPin);
  int mV = refV * adcSample;

  

  Serial.printf("mVMin:%d\tmVMax:%d\tmV:%d\n", SerialPlotterLow, SerialPlotterHigh, mV);

}
