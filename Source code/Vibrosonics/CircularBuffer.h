#ifndef Circular_Buffer_h
#define Circular_Buffer_h

#include <cstddef>
#include <cstdint>

template <typename T>
class CircularBuffer {
private:
    T* bufferPtr;

    uint16_t bufferIndex;

    uint16_t numRows;
    uint16_t numCols;

public:
    CircularBuffer()
    {
        this->bufferPtr = NULL;
        this->numRows = 0;
        this->numCols = 0;
        this->bufferIndex = 0;
    };

    void setBuffer(T* bufferPtr, uint16_t numRows, uint16_t numColumns)
    {
        this->bufferPtr = bufferPtr;

        this->numRows = numRows;
        this->numCols = numCols;
    };

    T* getBuffer() { return this->bufferPtr; };
    uint16_t getNumRows() const { return this->numRows; };
    uint16_t getNumCols() const { return this->numCols; };
    uint16_t getCurrentIndex() const { return this->bufferIndex; };

    T* getCurrentData() { return this->bufferPtr + this->numRows * this->bufferIndex; };

    T* getData(int relativeIndex)
    {
        uint16_t _index = (this->bufferIndex + this->numCols + relativeIndex) % this->numCols;
        return this->bufferPtr + _index * this->numRows;
    };

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
