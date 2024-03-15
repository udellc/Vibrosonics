#include "Vibrosonics.h"
#include "Pulse.h"
#include <cmath>

int* AudioLabInputBuffer = AudioLab.getInputBuffer();

// example pulse
//Pulse aPulse = Pulse(0, SINE);

void Vibrosonics::init() 
{
  Serial.begin(115200);
  while (!Serial)
    ;
  delay(4000);

  AudioLab.init();

  // set pulse parameters
  // aPulse.setAttack(20, 10, 10);
  // aPulse.setAttackCurve(4.0);
  // aPulse.setSustain(100, 100, 5);
  // aPulse.setRelease(40, 50, 20);
  // aPulse.setReleaseCurve(0.5);
}

void Vibrosonics::update()
{
  if (AudioLab.ready()) {
    // call static function update() to update all pulses every window
    Pulse::update();

    // call start() to pulse, note: this doesn't have to wait for a pulse to finish,
    // can call start() again to restart the pulse...
    //aPulse.start();

    // perform FFT on AudioLab input buffer, using ArduinoFFT
    performFFT(AudioLabInputBuffer);

    // store frequencies computed by FFT into freqs array
    storeFFTData();

    // ensure that the mean energy of frequency magnitudes is above a certain threshold
    if (getMean(freqsCurrent, WINDOW_SIZE) > 15.0) {
      
      // floor frequency magnitudes below a certain threshold
      noiseFloor(freqsCurrent, 15.0);

      // use findPeaks function to find at most 16 peaks in frequency data
      findPeaks(freqsCurrent, 16);

      // maps amplitudes to 0-127 range by dividing amplitudes by the max amplitude sum 
      // (will divide by actual sum if it exceeds this threshold)
      mapAmplitudes(FFTPeaksAmp, MAX_NUM_PEAKS, 3000);

      // assign peaks to sine waves
      //assignWaves(FFTPeaks, FFTPeaksAmp, MAX_NUM_PEAKS);
    }

    // call AudioLab.synthesize() after all waves are set.
    AudioLab.synthesize();
    //AudioLab.printWaves();
  }
}

void Vibrosonics::processInput()
{
  performFFT(AudioLabInputBuffer);
  storeFFTData();
  noiseFloor(freqsCurrent, 15.0);

  return;
}

void Vibrosonics::processInputCB()
{
  performFFT(AudioLabInputBuffer);
  storeFFTDataCB();
  Serial.printf("Process Input");
  //noiseFloor(freqsCurrent, 15.0);

  return;
}

void Vibrosonics::analyze()
{  
  for(int i=0; i<numModules; i++)
  {
    modules[i]->doAnalysis();
  } 
}

void Vibrosonics::addModule(AnalysisModule* module)
{
  
  // set module parameters
  module->setInputArrays(freqsPrevious, freqsCurrent);
  
  // create new larger array for modules
  numModules++;
  AnalysisModule** newModules = new AnalysisModule*[numModules];
  
  // copy modules over and add new module
  for(int i=0; i<numModules-1; i++){
    newModules[i] = modules[i];
  }
  newModules[numModules-1] = module;
  
  // free old modules array and store reference to new modules array
  delete [] modules;
  modules = newModules;
}

// maps frequencies from 0-SAMPLE_RATE to haptic range (0-250)
void Vibrosonics::mapFrequenciesLinear(float* freqData, int dataLength)
{
  float freqRatio;
  for (int i = 0; i < dataLength; i++) {
    freqRatio = freqData[i] / (SAMPLE_RATE>>1);
    freqData[i] = round(freqRatio * 250) + 20;
  }
}

// maps frequencies to haptic range (0-250Hz) using an exponent 
void Vibrosonics::mapFrequenciesExponential(float* freqData, int dataLength, float exp)
{
  float freqRatio;
  for (int i=0; i < dataLength; i++) {
    if (freqData[i] <= 200) {continue;}  // freq already within range
    freqRatio = freqData[i] / (SAMPLE_RATE>>1);
    freqData[i] = pow(freqRatio, exp) * 250;
  }
}