#include "CircularBuffer.h"
#include <Audiolab.h>

//****************************************************************************
// CONSTRUCTORS / DESTRUCTORS
//****************************************************************************

// Default constructor
// Results in a useless buffer; must call setNumAndSize or other parameter setting function
CircularBuffer::CircularBuffer()
{
    numBuffers = 0;
    bufferSize = 0;
    data = new float*[numBuffers];
    
    writeHead = 0;
    readHead = 0;
}

// Constructor with parameters
CircularBuffer::CircularBuffer(int numBuffers, int bufferSize)
{
    this->numBuffers = numBuffers;
    this->bufferSize = bufferSize;
    
    data = new float*[numBuffers];
    for(int i = 0; i < numBuffers; i++){
        data[i] = new float[bufferSize];
        for(int j=0; j<bufferSize; j++){
            data[i][j] = 0;
        }
    }
    
    writeHead = 0;
    readHead = 0;
}

// Destructor
// Should never get called in our use case, but done for completeness
CircularBuffer::~CircularBuffer()
{
    for (int i = 0; i < numBuffers; i++){
        delete [] data[i];
    }
    delete [] data;
}

//****************************************************************************
// SET PARAMETERS
//****************************************************************************

// Set the number of buffers and the size of each buffer
// If either parameter is changed, the data is deleted and re-allocated
// TODO: Copy and truncate or pad data instead of deleting it. 
//       This is not a problem for the current use -> buffer should be set up before use
//       Could be done for completeness anyway
void CircularBuffer::setNumAndSize(int num, int size)
{
    if(num != numBuffers || size != bufferSize)
    {
        for (int i = 0; i < numBuffers; i++){
            delete [] data[i];
        }
        delete [] data;
        
        numBuffers = num;
        bufferSize = size;
        data = new float*[numBuffers];
        for (int i = 0; i < numBuffers; i++){
            data[i] = new float[bufferSize];
            for(int j=0; j<bufferSize; j++){
                data[i][j] = 0;
            }
        }
    }
}

// Set the number of buffers
// Just calls setNumAndSize and passes the current buffer size
void CircularBuffer::setNumBuffers(int num)
{
    setNumAndSize(num, bufferSize);
}

// Set the size of the buffers
// Just calls setNumAndSize and passes the current number of buffers
void CircularBuffer::setBufferSize(int size)
{
    setNumAndSize(numBuffers, size);
}

//****************************************************************************
// MODULO
//****************************************************************************

// Custom modulo function for calculating buffer indices
// this function is necessary for calculating in-bound buffer indices
//   --> ordinary modulo can result in negative values

// if dividend is negative, increment by divisor until ordinary modulo can be taken
int CircularBuffer::smartModulo(int dividend, int divisor)
{
  if(divisor == 0){ return 0; }
  while(dividend < 0){
    dividend += divisor;
  }
  return dividend % divisor;
}


//****************************************************************************
// READ / WRITE DATA
//****************************************************************************

// Write data to the current buffer, overwrite the oldest buffer
// Current is incremented and wrapped around
void CircularBuffer::write(float* buffer, float freqWidth)
{
    for (int i = 0; i < bufferSize; i++){
        float value = buffer[i] * freqWidth;
        if(value < 25){ value = 0;}
        data[writeHead][i] = value;
    }
    readHead = smartModulo(writeHead, numBuffers);
    writeHead = smartModulo(writeHead + 1, numBuffers);
}

// Read data from the buffer at index idx
// idx is measured from the current buffer, not from 0
const float* CircularBuffer::getBuffer(int idx)
{ 
    idx = smartModulo(readHead + idx, numBuffers);
    
    const float* readOnlyCopy = data[idx];
    return readOnlyCopy;
}

// Get the current buffer
// This is equivalent to getBuffer(0), just more readable in code
const float* CircularBuffer::getCurrent()
{ 
    return getBuffer(0);
}

// Get the previous buffer
// This is equivalent to getBuffer(-1), just more readable in code
const float* CircularBuffer::getPrevious()
{ 
    return getBuffer(-1); 
}

// Unwind the circular buffer into a 2D array
// The 0 index is the most recent data, the 1 index is the previous data, etc.
// This is necessary for passing data to the analysis modules which expect a 2D array
const float** CircularBuffer::unwind()
{
    const float** unwoundData = new const float*[numBuffers];
    for(int i=0; i<numBuffers; i++){
        unwoundData[i] = getBuffer(-i);
    }
    return unwoundData;
}

// Print all the data in the buffer
void CircularBuffer::printAll()
{
    for(int i=0; i<numBuffers; i++){
        Serial.printf("Buffer[%d]: ", i);
        for(int j=0; j<bufferSize; j++){
            Serial.printf("%f, ", j);
        }
        Serial.printf("\n");
    }
}