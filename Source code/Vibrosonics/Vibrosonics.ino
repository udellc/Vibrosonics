#include "Vibrosonics.h"
#include "AnalysisModule.h"

Vibrosonics v = Vibrosonics();

TotalAmplitude totalAmp = TotalAmplitude();
MaxAmplitude maxAmp = MaxAmplitude();
MeanAmplitude meanAmp = MeanAmplitude();
Centroid cent = Centroid();
//SalientFreqs salFreqs = SalientFreqs();

void setup() {
  v.init();
  
  //total.setAnalysisFreqRange(0, 500);

  v.addModule(&totalAmp);
  v.addModule(&maxAmp);
  v.addModule(&meanAmp);
  v.addModule(&cent);
  //v.addModule(&salFreqs);
  
  Serial.printf("Ready\n");
}

void loop() {
  if (AudioLab.ready()) {

    v.processInput();
    v.analyze();

    Serial.printf("total: %f\n", totalAmp.getOutput());
    Serial.printf("max: %f\n", maxAmp.getOutput());
    Serial.printf("mean: %f\n", meanAmp.getOutput());
    Serial.printf("cent: %f\n", cent.getOutput());
    
    // map to outputs
    //assignWaves(/* major peaks output */); // additive synthesizer

    AudioLab.synthesize();
  } 
}
