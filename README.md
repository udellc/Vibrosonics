# Vibrosonics
### Experience audio through vibration.
**Vibrosonics** is an Arduino library that enables you to translate audio into
tactile vibrations in real-time.

[**Documentation (Doxygen)**](https://udellc.github.io/Vibrosonics/) \
**Hardware:** Adafruit ESP32 Feather, MAX9744 Amplifier board, TT25-8 puck
transducer, 3.5mm audio jack cable \
**Dependencies:** [AudioLab](https://github.com/synytsim/AudioLab) 
· [AudioPrism](https://github.com/udellc/AudioPrism) 
· [Fast4ier](https://github.com/jmerc77/Fast4ier)

## Table of Contents
- [Overview](#overview)
- [Installation](#installation-arduino-ide)
- [Library Architecture](#library-architecture)
- [Examples](#examples)
- [Contributors](#contributors)

## Overview
The goal of the Vibrosonics project is to enable those who struggle with or
cannot hear to enjoy audio-based entertainment in a different way as well as
potentially enhance audio-related experiences for enthusiasts by adding the
sensation of touch.

The aim is to create an easily wearable, portable device that can take in audio
signals and convert them to vibrations in the haptic range. These vibrations
should accurately portray different pitches and intensities of sound. 

We have partnered with [Cymaspace](https://www.cymaspace.org/), an organization
whose goal is to make culture and arts accessible for the deaf and hard of
hearing community. This group of people makes up our primary audience, as
haptic feedback can be used to replace or enhance audio. Some secondary users
would be employers whose work environments make pure audio based communication
difficult. They could instead receive important audio cues through haptics.
Lastly, events such as silent raves, or games that use alternate reality could
be interested in using this product to add a new sense. In all these scenarios,
the use of haptic vibrations can be used to enhance user experiences in an
accessible and interactive way. 

Currently, Apple has a feature called “music haptics”. Similarly to
Vibrosonics, this provides a way for users to experience music through tactile
vibrations. You can only use this feature with an Apple iPhone with iOS 18 or
higher, and it only works for music. While our product is currently also
focused on music, we hope that this will someday be used in everyday life as
well. Our product is also compatible for use on Windows, Linux, and Mac. 

## Installation (Arduino IDE)
### 1. Install Arduino IDE and ESP32 Board Support
- Download [Arduino IDE](https://www.arduino.cc/en/software/)
- Go to **Tools > Boards > Boards Manager**
- Search and install `esp32` by **Espressif Systems**

![Installing the board manager](/docs/images/Board_library.png)

### 2. Set Board and Ports
- Plug in the board (you may need to install USB drivers)
- Go to the top drop down menu and click `Select other board and port...` 
- In the pop up window, search for `Adafruit ESP32 Feather` and select it along
with `COMX` for port where `X` is a number 0-6
    - You may need to download a driver to see the port ([Windows Tutorial](https://randomnerdtutorials.com/install-esp32-esp8266-usb-drivers-cp210x-windows/))

![The other board and port drop down](/docs/images/Confirm_board.png)

### 3. Add Libraries
Download the zip files of Vibrosonics and each dependency (**Code > Download ZIP**):
- [AudioLab](https://github.com/synytsim/AudioLab)
- [AudioPrism](https://github.com/udellc/AudioPrism) 
- [Fast4ier](https://github.com/jmerc77/Fast4ier)

Import into Arduino via: \
**Sketch > Include Library > Add .ZIP library (repeat for each zip)**
![Adding personal libraries](/docs/images/Add_library.png)

### 4. Upload a Sketch
You should be ready to use the example sketches or create your own! To upload a
sketch to the Vibrosonics hardware:
- Plug in the ESP32 via USB
- Open your own sketch or one of our examples from **File > Examples >
Vibrosonics > \[Example Name]**
- Press the upload (➜) in the top left to compile and upload your sketch
![Uploading a sketch to the arduino](/docs/images/Upload_sketch.png)

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
- Start with the `Template` example to see basic use of the API and
for a starting point for your own program
- The `Grains` example demonstrates using the provided classes for granular
synthesis with a frequency and amplitude sweep, duration changes and wave shape
variations
- `Percussion` showcases our current percussion detection method, which uses a specially filtered frequency domain representation as input for an AudioPrism percussion module
- `Mirror mode` is our example demonstrating a combination of multiple
techniques to provide a real-time translation of music to tactile feedback.
Look here for an in-depth example utilizing the full capabilities of our
library

## Contributors
### 2024-25 Software Team
- [Walt Bringenberg](https://github.com/wwaltb)
- [Ben Kahl](https://github.com/ben-kahl)
- [Keith Reinhardt](https://github.com/reinhake)
- [Ashton Tilton](https://github.com/amputee20000)
- [Julia Yang](https://github.com/jjuliayang)

[LinkedIn Pages](https://dot.cards/vibrosonicscs)

### Special Thanks
 - Dr. Chet Udell (Project leader and manager)
 - Nick Synytsia (Developed AudioLab and advised software development for 2024-25)
 - Alex Synytsia (Participated in hardware development during 2022-23)
 - Vincent Vaughn (Advised software and hardware development)
