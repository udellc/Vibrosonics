#include "Vibrosonics.h"

Vibrosonics v = Vibrosonics();
TotalAmplitude total = TotalAmplitude();

//MajorPeaks majorPeaks = MajorPeaks(/* */);

void setup() {
  v.init();
  v.addModule(&total);

  Serial.printf("Setup");
}

void loop() {
  if (AudioLab.ready()) {

    v.processInput();
    Serial.printf("ready\n");
    v.analyze();

    Serial.printf("beforeOutput\n");
    float tot = total.getOutput();
    Serial.printf("%f\n", tot);
    // other modules etc

    // map to outputs
    //assignWaves(/* major peaks output */); // additive synthesizer

    AudioLab.synthesize();
  } 
}