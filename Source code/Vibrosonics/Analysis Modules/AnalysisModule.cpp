#include "AnalysisModule.h"

template <typename T> void AnalysisModule<T>::setWindowSize(int size)
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
}

template <typename T> int AnalysisModule<T>::getWindowSize()
{
  return windowSize;
}

template <typename T> void AnalysisModule<T>::setInputFreqs(int** freqs)
{
  input = freqs;
}

template <typename T> int** AnalysisModule<T>::getInputFreqs()
{
  return input;
}