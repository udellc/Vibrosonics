#ifndef Mean_Amplitude_h
#define Mean_Amplitude_h

#include "AnalysisModule.h"
#include "TotalAmplitude.h"

class MeanAmplitude : public AnalysisModule<float>
{
private:
    TotalAmplitude total = TotalAmplitude();

public:
    void doAnalysis()
    {
        total.doAnalysis();
        float tot = total.getOutput();
        output = tot / windowSizeBy2;
    }

    void setWindowSize(int size)
    {   
        // if size is not a non-zero power of two, return
        if((size <= 0 || ((size & (size-1)) != 0)))
        {
            return;
        }

        // size is a non-zero power of two
        // set windowSize to size
        windowSize = size;
        windowSizeBy2 = windowSize>>1;
        total.setWindowSize(size);
    }

    void setInputFreqs(int** freqs)
    {
        total.setInputFreqs(freqs);
        input = freqs;
    }

};
#endif