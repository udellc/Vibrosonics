#include "Vibrosonics.h"
#include "AnalysisModule.h"
#include "CircularBuffer.h"
#include "Pulse.h"

Vibrosonics v = Vibrosonics();

// declare modules
MajorPeaks mp = MajorPeaks(8);
MajorPeaks high = MajorPeaks(4);
PercussionDetection pd = PercussionDetection(500, 500, 0.6);
SalientFreqs sf = SalientFreqs();
BreadSlicer bs = BreadSlicer();
Pulse pulses[8];

void setup() {
  v.init();

  high.setAnalysisRangeByFreq(400, 1200);
  pd.setAnalysisRangeByFreq(1000, 4000);
  v.addModule(&pd);
  v.addModule(&high);
  v.addModule(&mp);
  v.addModule(&sf);
  v.addModule(&bs);
  int bands[] = {0, 200, 500, 2000, 4000};
  bs.setBands(bands, 4);

  Serial.printf("Ready\n");
}
void loop() {
  if (AudioLab.ready()) {

    v.processInputCB();
    v.analyze();

    int* testFreqs = sf.getOutput();
    sf.printSalientFreqs();
    //Serial.printf("Freq: %d\n", testFreqs[0]);

    float** mpOutput = mp.getOutput();
    float* amps = bs.getOutput();

    Serial.printf("BreadSlicer: ");
    for (int i = 0; i < 4; i++) {
      Serial.printf("%03g, ", amps[i]);
    }

    v.mapAmplitudes(mpOutput[1], 8, 1000);

    for (int i = 0; i < 8; i++) {
      if (mpOutput[0][i] > 120) break;

      if (pulses[i].getPulseState() == READY) {
        pulses[i].setAttack(mpOutput[0][i], mpOutput[1][i] * 0.8, 1);
        pulses[i].setSustain(mpOutput[0][i], mpOutput[1][i] * 1.0, 2);
        pulses[i].setRelease(mpOutput[0][i] * 0, mpOutput[1][i] * 0.2, 3);
        pulses[i].setReleaseCurve(1.5);
        pulses[i].start();
      }
    }
    Pulse::update();

    v.mapFrequenciesExponential(mpOutput[0], 8, .5);
    
    float** high_out = high.getOutput();
    v.mapFrequenciesExponential(high_out[0], 4, 0.5);
    v.mapAmplitudes(high_out[1], 4, 1000);
    if(pd.getOutput()){
      for(int i=0; i<4; i++){
        high_out[1][i] *= 1.5;
      }
    }
    for(int i=0; i<4; i++){
      v.assignWave(high_out[0][i], high_out[1][i], 1);
    }
    
    AudioLab.synthesize();
    //AudioLab.printWaves();
  } 
}