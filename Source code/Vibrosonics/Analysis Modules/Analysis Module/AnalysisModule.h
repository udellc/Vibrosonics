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
#include <AudioLab.h>
//#define WINDOW_SIZE = 256;
//#define WINDOW_SIZE_BY_2 = 128;
//#define FREQ_WIDTH = 8192 / 256;

template <typename T> class AnalysisModule
{
protected:
    // global constants from audio lab
    int windowSize = WINDOW_SIZE;
    int windowSizeBy2 = windowSize >> 1;
    
    int freqRes = SAMPLE_RATE / WINDOW_SIZE;
    int freqWidth = WINDOW_SIZE / SAMPLE_RATE;

    // frequency range
    int lowerBinBound = 0;
    int upperBinBound = windowSizeBy2;

    // reference to submodules (used to automatically propagate base class parameters)
    //AnalysisModule* submodules;

    // result of most recent analysis
    T output;

public:
    // input arrays from Vibrosonics
    int** pastWindows;
    float* curWindow;

    // pure virtual function to be implemented by dervied classes
    virtual void doAnalysis() = 0;
    
    // return output of most recent analysis
    T getOutput() {
        return output;
    }

    // if a module needs submodules, call this function in the parent module's constructor 
    // this is necessary to automatically propagate base class parameters like freq range
    //void addSubmodule(AnalysisModule* module);

    // provide a references to the input arrays
    void setInputArrays(int** pastWindows, int* curWindow);

    // set the frequency range of analysis
    void setAnalysisFreqRange(int lower, int upper);
};

/*
EXAMPLE DERIVED CLASS

class moduleName : public AnalysisModule<type>
{
public:
    doAnalysis()
    {
        // do analysis here
        // set output to result
        output = result;
    }
}
*/

/*
EXAMPLE DERIVED CLASS WITH SUBMODULES

class moduleName : public AnalysisModule<type>
{
protected:
    // declare submodules
    DerivedModule subModule1 = DerivedModule();
    DerivedModule subModule2 = DerivedModule();

public:
    moduleName()
    {
        // set submodule parameters
        subModule.setParams(...);

        // add submodules to array
        submodules = {subModule1, subModule2};
    }
    
    doAnalysis()
    {
        subModule1.doAnalysis();
        subModule2.doAnalysis();

        float result1 = subModule1.getOutput();
        float result2 = subModule2.getOutput();
        
        output = result1 * result2;
    }
}
*/

#endif