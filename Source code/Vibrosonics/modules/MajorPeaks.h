//============================================================================
// MODULE INFORMATION
//============================================================================
// Name        : MajorPeaks
// Return Type : float** (array of frequency and amplitude pairs)
// Description : MajorPeaks finds the N largest peaks in the current window
//               and returns an array of tuples containing the frequency and
//               amplitude of each peak. If there are fewer than N peaks, the
//               remaining elements in the array are padded with zeros.
//============================================================================

#ifndef Major_Peaks_h
#define Major_Peaks_h

#include "../AnalysisModule.h"

class MajorPeaks : public ModuleInterface<float**>
{
private:
    int maxNumPeaks = 8; // default number of peaks to find
    int numPeaks = 0;    // number of peaks found this iteration

    // temporary storage for calculating the output
    float* outputFrequencies = new float[windowSizeBy2>>1];
    float* outputAmplitudes = new float[windowSizeBy2>>1];

public:
    MajorPeaks(int n)
    {
        maxNumPeaks = n;
        output = new float*[2];
        output[0] = new float[maxNumPeaks];
        output[1] = new float[maxNumPeaks];
    }

    ~MajorPeaks()
    {
        // free all memory
        delete[] output[0];
        delete[] output[1];
        delete[] output;
        delete[] outputFrequencies;
        delete[] outputAmplitudes;
    }

  // reset peaks arrays
    void resetPeaksArrays()
    {
        numPeaks = 0;
        for(int i=0; i<windowSizeBy2>>1; i++)
        {
            outputFrequencies[i] = 0;
            outputAmplitudes[i] = 0;
        }
    }

    // find peaks in the current window
    // a peak is a freq. bin whose amplitude is greater than its neighbors
    void findPeaks()
    {
        // iterate through the frequency range
        for(int i=lowerBinBound+1; i<upperBinBound; i++)
        {
            // if the current bin is a peak, store its frequency and amplitude
            if(curWindow[i] > curWindow[i - 1] && 
               curWindow[i] > curWindow[i + 1])
            {
                outputFrequencies[numPeaks] = i * freqRes;
                outputAmplitudes[numPeaks] = curWindow[i];
                numPeaks++;
            }
        }
    }

    // remove the smallest peaks until numPeaks <= maxNumPeaks
    void trimPeaks()
    {
        while(numPeaks > maxNumPeaks)
        {
            float minAmp = outputAmplitudes[0];
            int minIndex = 0;
            
            // find the smallest peak
            for(int i=1; i<numPeaks; i++)
            {
                if(outputAmplitudes[i] < minAmp)
                {
                    minAmp = outputAmplitudes[i];
                    minIndex = i;
                }
            }
            
            // remove the smallest peak
            for(int i=minIndex; i<numPeaks-1; i++)
            {
                outputFrequencies[i] = outputFrequencies[i+1];
                outputAmplitudes[i] = outputAmplitudes[i+1];
            }
            numPeaks--;
        }
    }

    // copy peaks to the output arrays
    void storePeaks(){
        for(int i=0; i<maxNumPeaks; i++){
            // if there are fewer than maxNumPeaks peaks, pad array with zeros
            if(i < numPeaks)
            {
                output[0][i] = outputFrequencies[i];
                output[1][i] = outputAmplitudes[i];
            } else {
                output[0][i] = 0;
                output[1][i] = 0;
            }
        }
    }

    /*
    // for demo/debugging purposes
    void printOutput(){
        Serial.printf("[Freq, Amp]: ");
        for(int i=0; i<maxNumPeaks-1; i++){
            Serial.printf("[%g, %g], ", round(output[0][i]), round(output[1][i]));
        }
        Serial.printf("[%g, %g]\n", round(output[0][maxNumPeaks-1]), round(output[1][maxNumPeaks-1]));
    }
    */

    // perform the analysis
    void doAnalysis()
    {
        resetPeaksArrays();
        findPeaks();
        trimPeaks();
        storePeaks();
    }

};

#endif