#include "vibrosonics.h"

void IRAM_ATTR DAC_OUT(void) 
{
  //speakerMode();
  FFTMode();
  //dac.setChannel(SS_PIN, AD56X4_SETMODE_INPUT_DAC, AD56X4_CHANNEL_A, (word)(sinusoid[iterator]));
  //iterator = (iterator + 1) % SAMPLE_RATE;
}

void setup() 
{
  initialize(&DAC_OUT);
  //generateWave(440, sinusoid, 0.4);
}

void loop() 
{
  FFTMode_loop();
}