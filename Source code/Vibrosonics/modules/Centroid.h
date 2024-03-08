//============================================================================
// MODULE INFORMATION
//============================================================================
// Name        : Centroid
// Return Type : int (center of mass of the frequency spectrum)
// Description : Analysis method that calculates the "center of mass" of the 
//               frequency spectrum. The output is calculated by summing the 
//               product of the frequency and amplitude of each bin and 
//               dividing that sum by the total amplitude of the spectrum.
//               The output of this module can be interpreted as a measure
//               of the brightness of the input audio.
//============================================================================

#ifndef Centroid_h
#define Centroid_h

#include "../AnalysisModule.h"
#include <cmath>

// Centroid inherits from the ModuleInterface with an int output type
class Centroid : public ModuleInterface<int>
{
public:
    int centroid;
    int freqResBy2 = freqRes/2;

    void doAnalysis()
    {
        // get the sum of amplitudes and sum of frequencies*amplitudes
        float ampSum = 0;
        float freqAmpSum = 0;
        int amp, freq;
        for (int i = lowerBinBound; i < upperBinBound; i++) 
        {
            amp = curWindow[i];
            freq = (i * freqRes + freqResBy2);  // use the center frequency of a bin
            ampSum += amp;
            freqAmpSum += freq * amp;
        }
        centroid = (ampSum == 0) ? 0 : std::ceil(freqAmpSum / ampSum);
        output = centroid;
    }
};

#endif