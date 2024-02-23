#include "Vibrosonics.h"
#include "AnalysisModule.h"

Vibrosonics v = Vibrosonics();

// declare modules
MajorPeaks mp = MajorPeaks(4);

void setup() {
  v.init();

  mp.setAnalysisRangeByFreq(0, 1000);
  v.addModule(&mp);

  Serial.printf("Ready\n");
}

void loop() {
  if (AudioLab.ready()) {

    v.processInput();
    v.analyze();

    mp.printOutput();

    AudioLab.synthesize();
  } 
}
