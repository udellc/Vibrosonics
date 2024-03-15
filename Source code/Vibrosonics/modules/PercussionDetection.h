//============================================================================
// MODULE INFORMATION
//============================================================================
// Name        : PercussionDetection
// Return Type : bool (percussion detected in the current window)
// Description : This module is used to detect the presence of noisy 
//               transients. It uses the TotalAmplitude, DeltaAmplitude, and 
//               Noisiness modules to determine if a percussive sound is 
//               present. The thresholds for each of these modules can be set 
//               by the user to better fit the specific qualities of the input 
//               signal.
//----------------------------------------------------------------------------
// TIPS FOR USE: Choosing the proper thresholds for TotalAmplitude, Delta-
//               Amplitude, and Noisiness is crucial for accurate percussion
//               detection, becasue this module can be sensitive to the 
//               particular mixing qualities of the input signal.
//
//               Setting low threshold will improve the detection of
//               percussion, but may also increase the number of false
//               positives. 
//
//               Setting thresholds higher will decrease the number of false 
//               positives, but may also decrease the number of true positives. 
//
//               Setting a threshold to 0 will effectively disable the 
//               corresponding submodule.
//
//               In general, limiting the frequency range of this module to
//               only the higher frequencies of the input signal will improve
//               detection accuracy. Mid and low frequencies are often
//               cluttered with periodic elements, which can obscure
//               percussion or trigger false positives.
//============================================================================

#ifndef Percussion_Detection_h
#define Percussion_Detection_h

#include "../AnalysisModule.h"
#include "TotalAmplitude.h"
#include "Noisiness.h"

// TODO: enable delta analysis when input buffer is fixed

// PercussionDetection inherits from the ModuleInterface with a bool output type
// this submodule contains three submodules:
// Submodule 1: TotalAmplitude -- an indicator of the overall loudness of the current window
// Submodule 2: DeltaAmplitude -- an indicator of the change in loudness between the current and previous windows
//              (percussion is expected to have a high delta amplitude compared to sustained audio)
// Submodule 3: Noisiness -- an indicator of the noisiness of the current window
//              (percussion is expected to be noisier than periodic instruments like synths, pianos, strings, etc.) 
class PercussionDetection : public ModuleInterface<bool>
{
private:
    // submodules are made private so they cannot be accessed outside of the parent module
    
    // the TotalAmplitude submodule is taken as an indicator of the overall loudness of the input signal
    // the output of this submodule is compared against loudness_threshold
    // percussion is assumed to be louder than most sustained musical elements
    TotalAmplitude totalAmp = TotalAmplitude();
    
    // the loudness threshold is the minimum amplitude required for a window to be considered percussive
    float loudness_threshold = 60.0;

    // the DeltaAmplitude submodule outputs the change in loudness between the current and previous windows
    // the output of this submodule is compared against delta_threshold
    // percussion is characterized by a sharp increase in loudness, so a high delta amplitude is expected
    //DeltaAmplitude deltaAmp = DeltaAmplitude();

    // the delta threshold is the minimum change in amplitude required for a window to be considered percussive
    // by default, the delta threshold is correlated with the loudness threshold by a multiplicative factor 0.8
    float delta_threshold = 0.8 * loudness_threshold;
    
    // the Noisiness submodule gives an indication of how evenly spread energy is across the frequency spectrum
    // the output of this submodule is compared against noise_threshold
    // percussion is expected to be noisy, having energy spread evenly across the audio spectrum
    Noisiness noise = Noisiness();

    // the noise threshold is the minimum noisiness required for a window to be considered percussive
    float noise_threshold = 0.85; // possible range: 0.0 to 1.0

public:
    // constructor
    // a constructor is necessary for modules containing submodules
    // all submodules must be registered with the parent in the constructor
    PercussionDetection()
    {
        this->addSubmodule(&totalAmp);      // register the TotalAmplitude submodule
        //this->addSubmodule(&deltaAmp);    // register the DeltaAmplitude submodule
        this->addSubmodule(&noise);         // register the Noisiness submodule
    }
    
    void doAnalysis()
    {
        // run the submodules' analysses
        totalAmp.doAnalysis();
        //deltaAmp.doAnalysis();
        noise.doAnalysis();

        // retrieve the outputs of the submodules
        float total = totalAmp.getOutput();
        //float delta = deltaAmp.getOutput();
        float noiseOutput = noise.getOutput();

        // predict percussion is present if all three submodule's outputs are above their threshold values
        // an output of true indicates that percussion is predicted to be present in the current window
        // an output of false indicates that percussion is not predicted to be present in the current window
        // the output of this module can be retrieved by calling getOutput() after analysis
        output = (total >= loudness_threshold) /*&& (delta >= delta_threshold) */&& (noiseOutput >= noise_threshold);
    }
};
#endif