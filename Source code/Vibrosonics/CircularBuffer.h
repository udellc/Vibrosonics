#ifndef Circular_Buffer_h
#define Circular_Buffer_h

class CircularBuffer 
{
private:
    // parameters
    int numBuffers;
    int bufferSize;

    // data
    int readHead;
    int writeHead;
    float **data;

public:
    // constructors / destructor
    CircularBuffer();
    CircularBuffer(int numBuffers, int bufferSize);
    ~CircularBuffer();

    // paramter setting
    void setNumAndSize(int num, int size);
    void setNumBuffers(int num);
    void setBufferSize(int size);

    // modulo
    int smartModulo(int dividend, int divisor);

    // data access
    void write(float* buffer, float freqWidth);
    const float* getBuffer(int idx);
    const float* getCurrent();
    const float* getPrevious();
    const float** unwind();
    
    // print buffer
    void printAll();
    
};

#endif