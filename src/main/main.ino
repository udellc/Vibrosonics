/***************************************************************
 * FILE: main.ino
 * 
 * DATE: 11/4/2025
 * 
 * DESCRIPTION: Entry point for starting the Vibrosonics audio
 * analysis and web app.
 * 
 * AUTHOR: Ivan Wong
 ***************************************************************/

#include "webServer.h"
#include "networking.h"
#include "fileSys.h"
#include "config.h"

#ifdef ENABLE_VAPI

#include "VibrosonicsAPI.h"
#define NUM_PEAKS 32

// Vibrosonics audio analysis globals
static VibrosonicsAPI vapi = VibrosonicsAPI();
float windowData[WINDOW_SIZE_BY_2];
Spectrogram processedSpectrogram = Spectrogram(2, WINDOW_SIZE_OVERLAP);
ModuleGroup modules = ModuleGroup(&processedSpectrogram);
MajorPeaks majorPeaks = MajorPeaks(NUM_PEAKS);

#endif

/**
 * @brief 
 * 
 */
void setup()
{
  bool success = true;

  Serial.begin(115200);
  success &= FileSys::init();
  success &= Networking::init();
  success &= WebServer::init();

  // On setup failure, do nothing
  if (!success)
  {
    Serial.println("Setup failure. Looping...");
    
    while (true)
      delay(1000);
  }
  #ifdef ENABLE_VAPI
    Serial.println("Initializing VAPI")
    vapi.init();
    majorPeaks.setWindowSize(WINDOW_SIZE_OVERLAP);
    majorPeaks.setSpectrogram(&spectrogram);
  #endif
}

/**
 * @brief 
 * 
 */
void loop() {
  #ifdef ENABLE_VAPI
    // skip if new audio window has not been recorded
    if (!vapi.isAudioLabReady())
    {
      return;
    }
  #endif
  if (WebServer::hasQueuedRequest())
  {
    Serial.println("In here!!!");
    uint cur, prev = micros();
    
    #ifdef ENABLE_VAPI
      vapi.pause();
    #endif

    WebServer::processRequest();
    
    #ifdef ENABLE_VAPI
      vapi.resume();
    #endif
    
    cur = micros();
    Serial.print("It took ");
    Serial.print(cur - prev);
    Serial.println("microseconds to execute request");
  }
  #ifdef ENABLE_VAPI
    // process the raw audio signal into frequency domain data
    vapi.processAudioInput(windowData);
    vapi.noiseFloorCFAR(windowData, 4, 1, 1.8);
    spectrogram.pushWindow(windowData);
    majorPeaks.doAnalysis();
    synthesizePeaks(&majorPeaks);
    AudioLab.synthesize();
  #endif
}

#ifdef ENABLE_VAPI
int interpolateAroundPeak(float* data, int indexOfPeak)
{
  float prePeak = indexOfPeak == 0 ? 0.0 : data[indexOfPeak - 1];
  float atPeak = data[indexOfPeak];
  float postPeak = indexOfPeak == WINDOW_SIZE_BY_2 ? 0.0 : data[indexOfPeak + 1];
  // summing around the index of maximum amplitude to normalize magnitudeOfChange
  float peakSum = prePeak + atPeak + postPeak;
  // interpolating the direction and magnitude of change, and normalizing from -1.0 to 1.0
  float magnitudeOfChange = ((atPeak + postPeak) - (atPeak + prePeak)) / (peakSum > 0.0 ? peakSum : 1.0);

  // return interpolated frequency
  return int(round((float(indexOfPeak) + magnitudeOfChange) * FREQ_RES));
}

void synthesizePeaks(MajorPeaks* peaks)
{
  float** peaksData = peaks->getOutput();
  // interpolate around peaks
  vapi.mapAmplitudes(peaksData[MP_AMP], NUM_PEAKS, 20000);

  for (int i = 0; i < NUM_PEAKS; i++) {
    int freq = interpolateAroundPeak(spectrogram.getCurrentWindow(), round(int(peaksData[MP_FREQ][i] * FREQ_WIDTH)));
    vapi.assignWave(freq, peaksData[MP_AMP][i], 0);
    vapi.assignWave(freq, peaksData[MP_AMP][i], 1);
  }
}
#endif
