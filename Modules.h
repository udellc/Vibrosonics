#include "modules/BreadSlicer.h"
#include "modules/Centroid.h"
#include "modules/DeltaAmplitudes.h"
#include "modules/MaxAmplitude.h"
#include "modules/MeanAmplitude.h"
#include "modules/Noisiness.h"
#include "modules/PercussionDetection.h"
#include "modules/SalientFreqs.h"
#include "modules/TotalAmplitude.h"
#include <AudioPrism.h>

MajorPeaks majorPeaks = MajorPeaks();
BreadSlicer breadSlicer = BreadSlicer();
Centroid centroid = Centroid();
DeltaAmplitudes deltaAmplitudes = DeltaAmplitudes();
MaxAmplitude maxAmplitude = MaxAmplitude();
MeanAmplitude meanAmplitude = MeanAmplitude();
Noisiness noisiness = Noisiness();
PercussionDetection percussionDetection = PercussionDetection();
SalientFreqs salientFreqs =SalientFreqs();
TotalAmplitude totalAmplitude = TotalAmplitude();
