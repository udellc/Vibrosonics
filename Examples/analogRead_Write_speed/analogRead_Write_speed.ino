/*
  This code gets the average speed of analogRead() over 10,000 continuous samples, prints that value to console. Then does the same for analogWrite()
*/

#include <Arduino.h>

#define INPUT_PIN A2
#define OUTPUT_PIN A0

void setup() {
  // set baud rate
  Serial.begin(115200);
  // wait for Serial to set up
  while (!Serial)
    ;
  Serial.println("Ready");
  delay(1000);
  
  // get average analog read time over 10,0000 continuous samples
  unsigned long time = micros();
  for (int i = 0; i < 10000; i++) {
    analogRead(INPUT_PIN);
  }
  time = micros() - time;
  Serial.printf("Average analogRead() time(microseconds): %d\n", int(round(time / 10000.0)));
  // get average analog write time over 10,000 continous samples
  time = micros();
  for (int i = 0; i < 10000; i++) {
    analogWrite(OUTPUT_PIN, 0);
  }
  time = micros() - time;
  Serial.printf("Average analogWrite() time(microseconds): %d\n", int(round(time / 10000.0)));

}

void loop() {}
