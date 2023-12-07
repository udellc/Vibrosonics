#include "Vibrosonics.h"

/*/
########################################################
  Functions related to FFT analysis
########################################################
/*/

void Vibrosonics::pullSamples(void) {
  for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
    vReal[i] = AUD_IN_BUFFER[i];
    //Serial.println(vReal[i]);
  }
}

void Vibrosonics::pullSamples(float* output) {
  for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
    output[i] = float(AUD_IN_BUFFER[i]);
  }
}

void Vibrosonics::performFFT(void) {
  for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
    vImag[i] = 0.0;
  }
  
  // use arduinoFFT
  FFT.DCRemoval();                  // DC Removal to reduce noise
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
  FFT.Compute(FFT_FORWARD);             // Compute FFT
  FFT.ComplexToMagnitude();             // Compute frequency magnitudes
}


void Vibrosonics::performFFT(float *input) {
  for (int i = 0; i < FFT_WINDOW_SIZE; i++) {
    vReal[i] = input[i];
    vImag[i] = 0.0;
  }
  
  // use arduinoFFT
  FFT.DCRemoval();                  // DC Removal to reduce noise
  FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);  // Apply windowing function to data
  FFT.Compute(FFT_FORWARD);             // Compute FFT
  FFT.ComplexToMagnitude();             // Compute frequency magnitudes
}
