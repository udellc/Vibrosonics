#include "Vibrosonics.h"

Vibrosonics v = Vibrosonics();
TotatAmplitude total = TotalAmplitude();
v.addModule(&total);

//MajorPeaks majorPeaks = MajorPeaks(/* */);

void setup() {
  v.init();

}

void loop() {
  if (AudioLab.ready()) {

    v.processInput();
    v.analyze();

    float tot = total.getOutput();
    Serial.printf("%f", tot);
    // other modules etc

    // map to outputs
    //assignWaves(/* major peaks output */); // additive synthesizer

    AudioLab.synthesize();
  } 
}
