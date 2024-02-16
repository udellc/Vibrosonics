#ifndef Analysis_Module_h
#define Analysis_Module_h

#include <math.h>

// include derived analysis modules so they can be used by including this file
/*
#include "../Modules/BreadSlicer.h"
#include "../Modules/MajorPeaks.h"
#include "../Modules/PercussionDetection.h"

#include "../Modules/SalientFreqs.h"
#include "../Modules/Noisiness.h"

#include "../Modules/MaxAmplitude.h"
#include "../Modules/MeanAmplitude.h"
#include "../Modules/TotalAmplitude.h"
#include "../Modules/Centroid.h"
#include "../Modules/DeltaAmplitudes.h"
*/
//#include <AudioLab.h>
#define WINDOW_SIZE 256
#define SAMPLE_RATE 8192

class AnalysisModule
{
protected:
    
    // important constants
    int windowSize = WINDOW_SIZE;
    int windowSizeBy2 = windowSize >> 1;
    int freqRes = SAMPLE_RATE / WINDOW_SIZE;
    int freqWidth = WINDOW_SIZE / SAMPLE_RATE;
    
    // input arrays
    int** pastWindows;
    int* curWindow;

    // frequency range
    int lowerBinBound = 0;
    int upperBinBound = windowSizeBy2;

    // reference to submodules (used to automatically propagate parameters)
    AnalysisModule* submodules;

public:
    // pure virtual analysis function to be implemented by dervied classes
    virtual void doAnalysis() = 0;

    // if a module needs submodules, call this function in the parent module's constructor 
    // this is necessary to automatically propagate base class parameters to submodules
    void addSubmodule(AnalysisModule* module);

    // provide a references to the input arrays
    void setInputArrays(int** pastWindows, int* curWindow);

    // set the frequency range to analyze 
    void setAnalysisFreqRange(int lower, int upper);
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

#endif