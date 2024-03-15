#ifndef Analysis_Module_h
#define Analysis_Module_h

#include <math.h>

#include <AudioLab.h>
//#define WINDOW_SIZE 256
//#define SAMPLE_RATE 8192

class AnalysisModule
{
protected:
    
    // important constants
    int sampleRate = 8192;
    int windowSize = 256;
    int windowSizeBy2 = windowSize >> 1;
    float freqRes = float(sampleRate) / float(windowSize);
    float freqWidth = float(windowSize) / float(sampleRate);

    // frequency range
    int lowerBinBound = 0;
    int upperBinBound = windowSizeBy2;

    // reference to submodules (used to automatically propagate parameters)
    int numSubmodules = 0;
    AnalysisModule** submodules;

public:
    // input arrays from Vibrosonics
    const float* pastWindow;
    const float* curWindow;

    // pure virtual function to be implemented by dervied classes
    virtual void doAnalysis() = 0;

    // if a module needs submodules, call this function in the parent module's constructor 
    // this is necessary to automatically propagate base class parameters to submodules
    void addSubmodule(AnalysisModule* module);

    // provide a references to the input arrays
    void setInputArrays(const float* past, const float* current);

    // set the frequency range to analyze 
    void setAnalysisRangeByFreq(int lowerFreq, int upperFreq);
    void setAnalysisRangeByBin(int lowerBin, int upperBin);
};

// interface for analysis module templatized components
// interface is necessary: if the AnalysisModule class is templated, you can't put it in arrays
// derived classes should inherit from this interface, not AnalysisModule
template <typename T> class ModuleInterface : public AnalysisModule
{
protected:
    T output; // result of most recent analysis

public:
    T getOutput(){ return output;}

};

// include derived analysis modules so they can be used by including this file
#include "modules/BreadSlicer.h"
#include "modules/MajorPeaks.h"
#include "modules/PercussionDetection.h"

#include "modules/SalientFreqs.h"
#include "modules/Noisiness.h"

#include "modules/MaxAmplitude.h"
#include "modules/MeanAmplitude.h"
#include "modules/TotalAmplitude.h"
#include "modules/Centroid.h"
#include "modules/DeltaAmplitudes.h"

#endif