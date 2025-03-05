#include "ModuleGroup.h"

ModuleGroup::ModuleGroup(Spectrogram<float>* spectrogram)
{
    this->spectrogram = spectrogram;
}

void ModuleGroup::addModule(AnalysisModule* module)
{
    module->setWindowSize(WINDOW_SIZE);
    module->setSampleRate(SAMPLE_RATE);
    module->setSpectrogram(this->spectrogram);

    this->modules.push_back(module);
}

void ModuleGroup::addModule(AnalysisModule* module, int lowerFreq, int upperFreq)
{
    module->setWindowSize(WINDOW_SIZE);
    module->setSampleRate(SAMPLE_RATE);
    module->setSpectrogram(this->spectrogram);
    module->setAnalysisRangeByFreq(lowerFreq, upperFreq);

    this->modules.push_back(module);
}

void ModuleGroup::setSpectrogram(Spectrogram<float>* Spectrogram)
{
    this->spectrogram = spectrogram;

    for (AnalysisModule* module : this->modules) {
        module->setSpectrogram(spectrogram);
    }
}

void ModuleGroup::runAnalysis()
{
    for (AnalysisModule* module : this->modules) {
        module->doAnalysis();
    }
}
