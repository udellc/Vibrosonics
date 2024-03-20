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
    // constructor with optional parameter to set the number of peaks to find
    MajorPeaks(int n)
    {
        // restrict the number of peaks to find to n
        maxNumPeaks = n;

        // allocate memory for output array
        // output[0] is an array of frequencies, indexed by peak number
        // output[1] is an array of amplitudes, indexed by peak number
        // peaks are always stored in order of increasing frequency
        // if there are fewer than maxNumPeaks peaks, the remaining elements are padded with zeros
        output = new float*[2];
        output[0] = new float[maxNumPeaks];
        output[1] = new float[maxNumPeaks];
    }

    // destructor
    // frees memory allocated for the output arrays and temporary storage
    ~MajorPeaks()
    {
        // free memory allocated for output arrays
        delete[] output[0]; // free the array of frequencies
        delete[] output[1]; // free the array of amplitudes
        delete[] output;    // free the array managing the arrays of frequencies and amplitudes

        // free temporary storage
        delete[] outputFrequencies;
        delete[] outputAmplitudes;
    }

    // reset peaks arrays
    // this function is called at the beginning of each call to perform analysis
    // the number of peaks is reset to zero, and the arrays are zeroed out
    // the arrays must be reset to zero to avoid storing peaks from previous analysis calls
    void resetPeaksArrays()
    {
        numPeaks = 0; // reset the number of peaks to zero

        // zero out the output arrays
        for(int i=0; i<windowSizeBy2>>1; i++)
        {
            outputFrequencies[i] = 0;
            outputAmplitudes[i] = 0;
        }
    }

    // find all peaks in the current window
    // a peak is a freq. bin whose amplitude is greater than its neighbors
    // this function does not limit itself to finding maxNumPeaks peaks
    // if more than maxNumPeaks peaks are found, the smallest peaks are removed with trimPeaks later
    // if fewer than maxNumPeaks peaks are found, the remaining peaks are padded with zeros
    // this function should only be called after resetPeaksArrays()
    void findPeaks(const float** input)
    {
        // iterate through the frequency range, excluding the first and last bins
        for(int i=lowerBinBound+1; i<upperBinBound; i++)
        {
            // if the current bin is a peak, store its frequency and amplitude
            if(input[CURR_WINDOW][i] > input[CURR_WINDOW][i - 1] && 
               input[CURR_WINDOW][i] > input[CURR_WINDOW][i + 1])
            {
                // store the frequency of the peak
                // the index is multiplied by freqRes to convert the bin number to a frequency value
                // freqRes is the frequency width of each bin
                outputFrequencies[numPeaks] = i * freqRes; 
                
                // store the amplitude of the peak
                outputAmplitudes[numPeaks] = input[CURR_WINDOW][i];

                // increment the number of peaks found to reflect the addition of this peak
                numPeaks++;
            }
        }
    }

    // remove the smallest peaks until numPeaks <= maxNumPeaks
    // if there are equal to or fewer than maxNumPeaks peaks, this function does nothing
    // this function is called directly after findPeaks()
    // larger peaks are shifted to the left in the temporary storage arrays to overwrite smaller peaks
    void trimPeaks()
    {
        // loop until numPeaks <= maxNumPeaks, removing the smallest peak each iteration
        while(numPeaks > maxNumPeaks)
        {
            float minAmp = outputAmplitudes[0];
            int minIndex = 0;
            
            // iterate through the peaks, storing the index of the peak with the smallest amplitude
            // the peak at this index will be removed in the next step
            for(int i=1; i<numPeaks; i++)
            {
                if(outputAmplitudes[i] < minAmp)
                {
                    minAmp = outputAmplitudes[i];
                    minIndex = i;
                }
            }
            
            // remove the peak with the smallest amplitude
            // removal is done by shifting all elements after the removed peak one index to the left
            for(int i=minIndex; i<numPeaks-1; i++)
            {
                outputFrequencies[i] = outputFrequencies[i+1];
                outputAmplitudes[i] = outputAmplitudes[i+1];
            }
            numPeaks--;
        }
    }

    // storePeaks() copies peaks from temporary storage to the actual output arrays
    // this fuction is called after trimPeaks(), so there can only be up to maxNumPeaks peaks to store
    // if there are fewer than maxNumPeaks peaks, the remaining elements are padded with zeros
    void storePeaks(){
        // copy peaks from temporary storage to the actual output arrays
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

    // for demo/debugging purposes
    void printOutput(){
        Serial.printf("[Freq, Amp]: ");
        for(int i=0; i<maxNumPeaks-1; i++){
            Serial.printf("[%03g, %03g], ", round(output[0][i]), round(output[1][i]));
        }
        Serial.printf("[%03g, %03g]\n", round(output[0][maxNumPeaks-1]), round(output[1][maxNumPeaks-1]));
    }

    // perform the 4 step analysis
    // 1. resetPeaksArrays() to clear the temporary storage and output arrays, and reset the number of peaks found
    // 2. findPeaks() to find all peaks in the current window (up to windowSizeBy2>>1 peaks)
    // 3. trimPeaks() to remove the smallest peaks until numPeaks <= maxNumPeaks
    // 4. storePeaks() to copy peaks from temporary storage to the actual output arrays
    // this is the function called by the analysis manager to perform the analysis
    // the output is a 2d array of floats, where output[0] is an array of frequencies and output[1] is an array of amplitudes
    // the output is indexed by peak number, and is always in order of lowest freq peak to highest freq peak
    void doAnalysis(const float** input)
    {
        resetPeaksArrays();
        findPeaks(input);
        trimPeaks();
        storePeaks();
    }

};

#endif