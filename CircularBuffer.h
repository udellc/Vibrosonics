/**
 * @file
 * Contains the declaration of the CircularBuffer class
*/

#ifndef Circular_Buffer_h
#define Circular_Buffer_h

#include <cstddef>
#include <cstdint>

/**
  * @brief Class for the circular buffer 
  * This class is used to create and store frequency data into a circular buffer. 
  * This will give us a spectrogram. 
  * Circular Buffer has been changed from being dynamic to being static. 
  * This change allows for imporvements in performance and memory usage.
*/

template <typename T>
class CircularBuffer {
private:
    T* bufferPtr;

    uint16_t bufferIndex;

    uint16_t numRows;
    uint16_t numCols;

public:
    /**
      * @brief Default constructor to initialize a new circular buffer 
      *
      * Creates a circular buffer and sets everything to 0 or NULL
    */
    CircularBuffer()
    {
        this->bufferPtr = NULL;
        this->numRows = 0;
        this->numCols = 0;
        this->bufferIndex = 0;
    };

    /**
      * @brief Overloaded constructor to initialize a new circular buffer 
      * 
      * Creates a circular buffer that uses the specified values for the 
      * buffer pointer, rows, and columns
      * 
      * @param bufferPtr buffer pointer 
      * @param numRows number of rows in the buffer
      * @param numCols number of columns in the buffer 
    */
    CircularBuffer(T* bufferPtr, uint16_t numRows, uint16_t numCols)
    {
        this->bufferPtr = bufferPtr;
        this->numRows = numRows;
        this->numCols = numCols;
        this->bufferIndex = 0;
    };

    /**
      * @brief updates the buffer pointer, number of rows, and number of columns
      * 
      * @param bufferPtr buffer pointer 
      * @param numRows number of rows in the buffer
      * @param numCols number of columns in the buffer 
    */
    void setBuffer(T* bufferPtr, uint16_t numRows, uint16_t numCols)
    {
        this->bufferPtr = bufferPtr;

        this->numRows = numRows;
        this->numCols = numCols;
    };

    /**
      * @brief getter function for the buffer pointer 
      * /return pointer 
    */
    T* getBuffer() { return this->bufferPtr; };

    /**
      * @brief getter function for the number of rows 
      * /return int 
    */
    uint16_t getNumRows() const { return this->numRows; };

    /**
      * @brief getter function for the number of columns 
      * /return int 
    */
    uint16_t getNumCols() const { return this->numCols; };

    /**
      * @brief getter function for the current index of the buffer 
      * /return int 
    */
    uint16_t getCurrentIndex() const { return this->bufferIndex; };

    /**
      * @brief gets the frequency data that is stored in a the current 
      * index of the circular buffer 
      * /return pointer
    */
    T* getCurrentData() { return this->bufferPtr + this->numRows * this->bufferIndex; };

    /**
      * @brief gets the frequency data that is stored in a specified index
      * 
      * @param relativeIndex index to get data for 
      * /return pointer
    */
    T* getData(int relativeIndex)
    {
        uint16_t _index = (this->bufferIndex + this->numCols + relativeIndex) % this->numCols;
        return this->bufferPtr + _index * this->numRows;
    };

    /**
      * @brief takes new data and adds it to the circular buffer 
      * If the buffer index reaches the maximum column index, it 
      * wraps around and overwrites the oldest data, setting the 
      * index back to 0
      * 
      * @param data buffer pointer to data 
    */
    void pushData(T* data)
    {
        this->bufferIndex += 1;
        if (this->bufferIndex == this->numCols)
            this->bufferIndex = 0;

        for (int i = 0; i < this->numRows; i++) {
            *((this->bufferPtr + i) + this->numRows * this->bufferIndex) = data[i];
        }
    };

    /**
      * @brief clears the buffer and resets everything back to 0 
    */
    void clearBuffer(void)
    {
        for (int t = 0; t < this->numCols; t++) {
            for (int f = 0; f < this->numRows; f++) {
                *((this->bufferPtr + f) + t * this->numRows) = 0;
            }
        }
        this->bufferIndex = 0;
    };
};

#endif
