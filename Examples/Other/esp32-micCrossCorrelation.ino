//#include "ESP32TimerInterrupt.h"
#include <arduinoFFT.h>
#include <Arduino.h>

#define MIC_IN A0
#define LED_PIN A5

// audio input settings
//const double SampleFreq = 8192;
//const int RecordTime = 4;
const uint16_t FFTWinSize = 2048;
const float correlationThresh = 0.5;

// timer handler
//bool IRAM_ATTR TimerHandler1(void * timerNo)
//{
//  RecordSample();
//
//  return true;
//}
//
//// timer interrupt
//ESP32Timer ITimer1(0);


const int SerialPlotterLow = 0;
const int SerialPlotterHigh = 3300;
// 12 bit ADC to voltage conversion
const float refV = 3300.0/4095.0;

// arrays used for FFTs
double vReal[FFTWinSize];
double vSum = 0.0;
//double vImag[FFTWinSize];

// array for template
double vTemplate[FFTWinSize];
double tempSum = 0.0;

//// object for performing fft
//arduinoFFT FFT = arduinoFFT(vReal, vImag, FFTWinSize, SampleFreq);

//// Number of microseconds to wait before recoding next sample
//const int InputSampleDelayTime = 1000000 / SampleFreq;

void setup() {
  Serial.begin(115200);
  
   // initialize pins
  pinMode(MIC_IN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  //ITimer1.attachInterruptInterval(InputSampleDelayTime, TimerHandler1);
  
  for (int i = 0; i < FFTWinSize; i++) {
    vTemplate[i] = 1.0;
    tempSum += 1.0;
  }
}

void loop() {
  while (Serial.available()) {
    char data = Serial.read();
    if (data == 'x') {
      Serial.println("Set template in 1 second");
      delay(1000);
      RecordSample();
      SetTemplate();
      Serial.println("Template set");
    }
  }
  RecordSample();
  if (EvaluateTemplate()) {
    Serial.println("Correlation detected");
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("No correlation detected");
    digitalWrite(LED_PIN, LOW);
  }

  delay(10);
}

void SetTemplate() {
  tempSum = vSum;
  for (int i = 0; i < FFTWinSize; i++) {
    vTemplate[i] = vReal[i];
  }
}

bool EvaluateTemplate() {
  double sum_vRealvTemp = 0.0;
  double squareSum_vReal = 0.0;
  double squareSum_vTemp = 0.0;
  
  for (int i = 0; i < FFTWinSize; i++) {
    sum_vRealvTemp = sum_vRealvTemp + vReal[i] * vTemplate[i];

    squareSum_vReal = squareSum_vReal + vReal[i] * vReal[i];
    squareSum_vTemp = squareSum_vTemp + vTemplate[i] * vTemplate[i];  
  }

  float correlation = (float)(FFTWinSize * sum_vRealvTemp - tempSum * vSum)
                        / sqrt((FFTWinSize * squareSum_vReal - tempSum * vSum) 
                          * (FFTWinSize * squareSum_vTemp - tempSum * vSum));

  return (correlation >= correlationThresh);
}

void RecordSample() {
  vSum = 0;
  for (int i = 0; i < FFTWinSize; i++) {
    double adcSample = analogRead(MIC_IN);
    adcSample = adcSample / 10.0;
    vReal[i] = adcSample;
    vSum += adcSample;

    //int mV = refV * adcSample;

    //Serial.printf("mVMin:%d\tmVMax:%d\tmV:%d\n", SerialPlotterLow, SerialPlotterHigh, mV);
  }
}
