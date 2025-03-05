#ifndef MODULE_GROUP_H
#define MODULE_GROUP_H

#include <vector>

#include <AudioLab.h>
#include <AudioPrism.h>

#include "Spectrogram.h"

class ModuleGroup {
public:
    ModuleGroup(Spectrogram<float>* spectrogram);

    void addModule(AnalysisModule* module);

    void addModule(AnalysisModule* module, int lowerFreq, int upperFreq);

    void setSpectrogram(Spectrogram<float>* Spectrogram);

    void runAnalysis();

private:
    Spectrogram<float>* spectrogram;
    std::vector<AnalysisModule*> modules;
};

#endif // MODULE_GROUP_H
