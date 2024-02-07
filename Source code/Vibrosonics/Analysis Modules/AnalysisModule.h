#ifndef Analysis_Module_h
#define Analysis_Module_h

template <typename T> class AnalysisModule
{
protected:
    int windowSize;
    int windowSizeBy2;
    int** input;
    T output;
public:
    // IDEA: eventually add freq range constraints
    virtual void doAnalysis() = 0;
    T getOutput(){ return output; };

    void setWindowSize(int size);
    int getWindowSize();
    void setInputFreqs(int** input);
    int** getInputFreqs();
};

#endif