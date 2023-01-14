#include <Arduino.h>
#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/sin2048_int8.h>
#include "arduinoFFT.h"
#include "AddSynth.h"
#include "esp32FFTtoParabolicBins.h"

void setup()
{
  Serial.begin(115200);

  FFTtoParaSetup();
  AddSynthSetup();
}

void loop()
{
  FFTtoParaLoop();
  AddSynthLoop();

}