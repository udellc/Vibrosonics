# Vibrosonics
### Experience audio through vibration
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
- [Contributors](#contributors)

## Overview
The goal of the Vibrosonics project is to enable those who struggle with or
cannot hear to enjoy audio-based entertainment in a different way as well as
potentially enhance audio-related experiences for enthusiasts by adding the
sensation of touch.

The aim is to create an easily wearable, portable device that can take in audio
signals and convert them to vibrations in the haptic range. These vibrations
should accurately portray different pitches and intensities of sound. 

We have partnered with Cymaspace, an organization whose goal is to make culture
and arts accessible for the deaf and hard of hearing community. This group of
people makes up our primary audience, as haptic feedback can be used to replace
or enhance audio. Some secondary users would be employers whose work
environments make pure audio based communication difficult. They could instead
receive important audio cues through haptics. Lastly, events such as silent
raves, or games that use alternate reality could be interested in using this
product to add a new sense. In all these scenarios, the use of haptic
vibrations can be used to enhance user experiences in an accessible and
interactive way. 

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

![The other board and port drop down](/docs/images/Confirm_board.png)

### 3. Add Libraries
Download the zip files of Vibrosonics and each dependency (**Code > Download ZIP**):
- [AudioLab](https://github.com/synytsim/AudioLab)
- [AudioPrism](https://github.com/udellc/AudioPrism) 
- [Fast4ier](https://github.com/jmerc77/Fast4ier)

Import into Arduino via: \
**Sketch > Include Library> Add .ZIP library (repeat for each zip)**
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
this. The first is AudioLab, which helps in signal processing and additive
synthesis. The second is Fast4ier, which is used to perform the Fast Fourier
Transform (FFT). Finally, we use the AudioPrism library to analyze the
frequency spectrum obtained from the FFT result.

We have created an API, \ref VibrosonicsAPI, that combines these libraries to
provide a simple interface for audio processing, analysis and synthesis.

Additionally, there is the \ref Grain class which provides a way to implement
granular synthesis, allowing for sound wave shaping through envelopes. There
are the \ref GrainList and \ref GrainNode classes that help you manage a
linked-list of grains.

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
