#include "Vibrosonics.h"
#include "Analysis Modules/Analysis Module/AnalysisModule.h"
#include "Analysis Modules/Modules/TotalAmplitude.h"

Vibrosonics v = Vibrosonics();
TotalAmplitude total1 = TotalAmplitude();
TotalAmplitude total2 = TotalAmplitude();

void setup() {
  v.init();
  Serial.printf("Ready\n");
  
  total2.setAnalysisFreqRange(0, 500);

  v.addModule(&total1);
  v.addModule(&total2);
  
  Serial.printf("Ready\n");
}

void loop() {
  if (AudioLab.ready()) {

    v.processInput();
    v.analyze();

    Serial.printf("total1: %f\n", total1.getOutput());
    Serial.printf("total2: %f\n", total2.getOutput());
    // other modules etc

    // map to outputs
    //assignWaves(/* major peaks output */); // additive synthesizer

    AudioLab.synthesize();
  } 
}
