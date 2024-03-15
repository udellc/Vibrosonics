#include "Vibrosonics.h"
#include "AnalysisModule.h"
#include "CircularBuffer.h"

Vibrosonics v = Vibrosonics();

// declare modules
MajorPeaks mpLow = MajorPeaks(2);
MajorPeaks mpHigh = MajorPeaks(2);


void setup() {
  v.init();

  mpLow.setAnalysisRangeByFreq(0, 250);
  mpHigh.setAnalysisRangeByFreq(251, 500);
  v.addModule(&mpLow);
  v.addModule(&mpHigh);

  Serial.printf("Ready\n");
}

void loop() {
  if (AudioLab.ready()) {

    v.processInputCB();
    v.analyze();

    float** mpLowOutput = new float*[2];
    mpLowOutput = mpLow.getOutput();

    float** mpHighOutput = new float*[2];
    mpHighOutput = mpHigh.getOutput();

    for (int i=0; i < 2; i++) {
      //mpLowOutput[0][i] *= 0.5;
      mpHighOutput[0][i] *= 0.5;
    }
 
    v.mapAmplitudes(mpLowOutput[1], 2, 500);
    v.mapAmplitudes(mpHighOutput[1], 2, 500);

    v.mapFrequenciesExponential(mpLowOutput[0], 2, .5);
    v.mapFrequenciesExponential(mpHighOutput[0], 2, .5);

    v.assignWaves(mpLowOutput[0], mpLowOutput[1], 2);
    v.assignWaves(mpHighOutput[0], mpHighOutput[1], 2);

    AudioLab.synthesize();
    AudioLab.printWaves();
  } 
}
