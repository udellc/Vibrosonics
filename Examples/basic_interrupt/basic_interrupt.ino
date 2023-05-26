/*
  This code uses a timer interrupt to print something to console every second.
*/

#include <Arduino.h>

// the variable the interrupt modifies whenever it is triggered
volatile int trigger = 0;

hw_timer_t *My_timer = NULL;

// this is the function that is called whenever the interrupt is triggered
void IRAM_ATTR onTimer(){
  trigger = 1;
}

void setup() {
  // set baud rate
  Serial.begin(115200);
  // wait for Serial to set up
  while (!Serial)
    ;
  Serial.println("Ready");
  
  // setup interrupt for audio sampling with a prescaler of 80
  // The ESP32 clock speed is 80MHz, therefore having the prescalar at 80 divides the clock resolution by 80
  // essentially the prescalar makes certain time calculations less confusing
  My_timer = timerBegin(0, 80, true);
  timerAttachInterrupt(My_timer, &onTimer, true);
  // trigger interrupt every second ie (CPU clock / prescalar / value) = interrupts per second => in this case: 80MHz / 80 / 1M = 1
  timerAlarmWrite(My_timer, 1000000, true);
  timerAlarmEnable(My_timer); // enable interrupt
}

void loop() {
  // this is not necassary, just purely for demonstating that it works via Serial
  if (trigger == 1) {
    trigger = 0;
    Serial.println("Interrupt triggered");
  }
}
