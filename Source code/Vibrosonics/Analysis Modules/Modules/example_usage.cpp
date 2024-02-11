// including analysis manager implicitly includes all modules
#include "AnalysisManager.h"

// Vibrosonics AKA...
// "AnalysisHandler" or just "Handler"?
// "AnalysisManager" or just "Manager"?
// "AnalysisController" or just "Controller"?
// "AnalysisEngine"
// "AnalysisHub"
AnalysisManager manager = AnalysisManager();

// instantiate majorpeaks module
MajorPekas majPeaks = MajorPeaks();

// instantiate percussion detection module
PercussionDetection percDet = PercussionDetection();

void setup()
{
    // PARAMETER SETTING AND MANAGER REGISTRATION CAN BE GLOBAL OR IN SETUP

    // set majorpeaks parameters
    majorpeaks.setNumPeaks(8);
    
    // register majorpeaks with the manager/handler/controller/whatever
    manager.addModule(MajorPeaks);
    
    // set percussion detection parameters
    percDet.setAmpThresh(5000);                 // look at loud volumes
    percDet.setDeltaThresh(2500);               // look at large volume increases
    percDet.setNoiseThresh(0.85);               // look at high noisiness
    percDet.setAnalysisFreqRange(2500, 4000);   // look at high frequencies
    
    // register percussion detection with the manager
    manager.addModule(percDet);
}

void loop()
{
    if(AudioLab.ready())
    {
        // do FFT and run analysis
        manager.performFFT();
        manager.analyzeFFT();

        /* Everything below is an example of hardcoding 
        connections from analysis results to output modes */

        // major peaks goes to additive synth
        int** peaks = MajorPeaks.getOutput();
        mapAmplitudes(peaks[0], majPeaks.getNumPeaks(), 3000);
        assignWaves(peaks[0], peaks[1],  majPeaks.getNumPeaks());

        // percussion detection goes to pulse generator
        if(percDet.getOutput() == true)
        {
            Pulse();
        }
    }
    AudioLab.synthesize();
}