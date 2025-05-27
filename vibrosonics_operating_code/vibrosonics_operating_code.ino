#include "vibrosonics.h"

void IRAM_ATTR DAC_OUT(void) 
{
  //speakerMode(channel[0]);
  FFTMode();
}

void setup() 
{
  initialize(&DAC_OUT, FM_ENABLE);
}

void loop() 
{
  FFTMode_loop(0.5);
}