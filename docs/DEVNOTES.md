# Developer Notes

## Table of Contents

- [Library Architecture](#library-architecture)
- [API Classes](#api-classes)
- [Examples](#examples)

## Library Architecture

The Vibrosonics library aims to streamline three processes to enable
translating audio into the haptic range: audio signal processing, audio
analysis and finally audio synthesis. We utilize a few libraries to achieve
this.

The first is AudioLab, which handles the audio signals between the external
hardware and the processor. It reads audio from the analog-to-digital
converters (ADC) and writes signals to the digital-to-analog converters (DAC)
for synthesis. The captured audio signals from the ADC are analyzed using the
other two library dependencies.

Fast4ier is used to perform the Fast Fourier Transform (FFT) on the signal
data, which converts it from a representation of the signal over time (time
domain) to a representation of how the signal is distributed over a range of
frequency bands (frequency domain). The bandwidth of the frequency domain, or
the max frequency, is equal to half of the sample rate of the ADC. Similarly,
the number of different frequency bands is equal to half of the window size of
the ADC.

AudioPrism is a library which analyzes the frequency domain data using various
algorithms. By filtering this data for different AudioPrism analysis modules,
we can detect certain elements of the audio signal. With music, this allows us
to detect percussion, isolate prominent vocals and melodies, and more. With
this analysis, distinct vibrations can be made to model elements of music by
re-synthesizing these findings through AudioLab translated down into the haptic
frequency range (0-230Hz).

### API Classes

- `VibrosonicsAPI`: This is the core class; it is a unified interface for audio
processing, analysis and synthesis
- `Grain`, `GrainList`, and `GrainNode`: These are the components for granular
synthesis. `Grain` is the main grain class, and the list and node classes provide a
way to manage a linked list of grains.

## Examples

We have an `examples` folder which contains individual programs that each
showcase a different feature or technique possible using Vibrosonics and
AudioPrism.

- Start with the `Template` example to see basic use of the API and for a
starting point for your own program.
- The `Grains` example demonstrates using the provided classes for granular
synthesis with a frequency and amplitude sweep, duration changes and wave shape
variations.
- `Percussion` showcases our current percussion detection method, which uses a
specially filtered frequency domain representation as input for an AudioPrism
percussion module. It utilizes grains to create haptic feedback corresponding
to the detected percussive hits.
- `Melody` is a similar example of strategic frequency domain processing, but
to bring out melodic elements of music. These elements are resynthesized by
translating the most prominent frequency peaks into the haptic range.
- `Vibrosonics` is our example demonstrating a combination of multiple
techniques (`Percussion` and `Melody`) to provide a real-time translation of
music to tactile feedback. Look here for an in-depth example utilizing the full
capabilities of our library
