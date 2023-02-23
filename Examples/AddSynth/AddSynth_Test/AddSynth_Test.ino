#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>

#define GAIN_ADDER 50
#define CONTROL_RATE 128

int waveCount = 0;

Oscil <2048, AUDIO_RATE> asinc(SIN2048_DATA); //asinc is the carrier wave. This is the sum of all other waves and the wave that should be outputted.

const int OscilCount = 5;  //The total number of waves. Modify this if more waves are added, or the program will segfault.

int gains[OscilCount] = { 0 };  //Set between 0 and 1.

int carrier;

int addSyntComplete = 0;

void setup() {
  Serial.begin(115200);

  startMozzi();
  asinc.setFreq(32);  //16 Hz - Base shaker sub-harmonic freq.

  gains[0] = 0;
  gains[1] = 0;  //Set amplitude here. The array index corresponds to the sin wave number. Use any value between 0-1 for amplitude. (Hardcoded for now)
  gains[2] = 0;
  gains[3] = 0;
  gains[4] = 0;

  while (!Serial)
    ;
  Serial.println("Ready");
}

void loop() {
  audioHook();
}

void updateControl() {
  //audioHook();
  Serial.printf("Min:%d\tMax:%d\tCarrier:%d", 0, 255, carrier);
  Serial.println();

  char dataT = '0';
  while (Serial.available()) {
    char dataT = Serial.read();
    if (dataT == 'n') {
      if (gains[0] > (int)255 - GAIN_ADDER) {
        gains[0] = 0;
      }
      else {
        gains[0] += GAIN_ADDER;
      }
    }
  }
}


int updateAudio() {
  return (gains[0] * asinc.next()) >> 8;

}
