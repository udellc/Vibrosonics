/**
 * @file
 * Contains the declaration of the CircularBuffer class
 */

#ifndef SPECTROGRAM_H
#define SPECTROGRAM_H

#include <cstddef>
#include <cstdint>

/**
 * Class for the circular buffer.
 *
 * This class is used to create and store frequency data into a circular
 * buffer. This will give us a spectrogram. Circular Buffer has been changed
 * from being dynamic to being static. This change allows for imporvements in
 * performance and memory usage.
 */
template <typename T>
class Spectrogram {
private:
    T* buffer;
    uint16_t numWindows;
    uint16_t numBins;
    uint16_t currIndex;

public:
    /**
     * Default constructor to initialize a new circular buffer.
     *
     * Creates a circular buffer and sets everything to 0 or NULL.
     */
    Spectrogram()
    {
        this->buffer = NULL;
        this->numWindows = 0;
        this->numBins = 0;
        this->currIndex = 0;
    };

    /**
     * Overloaded constructor to initialize a new circular buffer.
     *
     * Creates a circular buffer that uses the specified values for the
     * buffer pointer, rows, and columns.
     *
     * @param bufferPtr buffer pointer
     * @param numRows number of rows in the buffer
     * @param numCols number of columns in the buffer
     */
    Spectrogram(T* buffer, uint16_t numWindows, uint16_t numBins)
    {
        this->buffer = buffer;
        this->numWindows = numWindows;
        this->numBins = numBins;
        this->currIndex = 0;
    };

    /**
     * Updates the buffer pointer, number of rows, and number of columns.
     *
     * @param bufferPtr buffer pointer
     * @param numRows number of rows in the buffer
     * @param numCols number of columns in the buffer
     */
    void setBuffer(T* buffer, uint16_t numWindows, uint16_t numBins)
    {
        this->buffer = buffer;
        this->numWindows = numWindows;
        this->numBins = numBins;
    };

    /**
     * Getter function for the buffer pointer.
     */
    T* getBuffer() { return this->buffer; };

    /**
     * Getter function for the number of rows.
     */
    uint16_t getNumBins() const { return this->numBins; };

    /**
     * Getter function for the number of columns.
     */
    uint16_t getNumWindows() const { return this->numWindows; };

    /**
     * Getter function for the current index of the buffer.
     */
    uint16_t getCurrentIndex() const { return this->currIndex; };

    /**
     * Gets the frequency data that is stored at the current
     * index of the circular buffer.
     */
    T* getCurrentWindow()
    {
        return this->buffer + (this->currIndex * this->numBins);
    };

    /**
     * Gets the frequency data that is stored in a specified index
     *
     * @param relativeIndex index to get data for
     */
    T* getWindow(int relativeIndex)
    {
        // make sure that modulus math will work for negative values:
        while (relativeIndex < 0) {
            relativeIndex += this->numWindows;
        }
        uint16_t index = (this->currIndex + relativeIndex) % this->numWindows;
        return this->buffer + (index * this->numBins);
    };

    /**
     * Takes new data and adds it to the circular buffer
     * If the buffer index reaches the maximum column index, it
     * wraps around and overwrites the oldest data, setting the
     * index back to 0
     *
     * @param data buffer pointer to data
     */
    void pushWindow(T* data)
    {
        this->currIndex++;
        if (this->currIndex == this->numWindows)
            this->currIndex = 0;

        T* windowBuffer = this->buffer + (this->currIndex * this->numBins);
        for (int i = 0; i < this->numBins; i++) {
            windowBuffer[i] = data[i];
        }
    };

    /**
     * Clears the buffer and resets everything back to 0
     */
    void clearBuffer()
    {
        for (int i = 0; i < numWindows * numBins; i++) {
            buffer[i] = 0;
        }
        this->currIndex = 0;
    };
};

#endif // SPECTROGRAM
