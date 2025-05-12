# Vibrosonics

Welcome to Vibrosonics!

Detailed documentation can be viewed on the [Doxygen Page](https://udellc.github.io/Vibrosonics/)

Hardware: Adafruit ESP32 Feather, MAX9744 Amplifier board, TT25-8 puck transducer, 3.5mm audio jack cable

Library dependencies:

- [AudioLab](https://github.com/synytsim/AudioLab)

- [AudioPrism](https://github.com/udellc/AudioPrism)

- [Fast4ier](https://github.com/jmerc77/Fast4ier)


## What is it?

The goal of the Vibrosonics project is to enable those who struggle with or cannot hear to enjoy audio-based entertainment in a different way as well as potentially enhance audio-related experiences for enthusiasts by adding the sensation of touch. 

To that end, the aim is to create an easily wearable portable device than can take in audio signals and convert them to vibrations reflective of the intensity and pitch of the initial sound.

The majority of initial adoption would likely be from users for whom the product directly fills an unmet need. Within the context of this device, this will be clients who require haptic feedback as a replacement for audio. Examples may include those who are deaf or hard of hearing, or employers whose work environments make pure audio based communication difficult and require alternatives. Subsequent adopters are likely to be enthusiasts who will use the product to enhance events or activities they already partake in by adding a tactile aspect, such as in silent raves or virtual and alternate reality gaming. 

Currently, the market for similar devices is highly limited and undeveloped. This project seeks to expound upon what little exists by incorporating a higher range of translated audio frequencies along with more detailed audio-to-haptic translation into the final tactile experience. Ideally, this product would progressively gain new users by becoming a serious consideration for the aforementioned groups that require an alternative to pure audio. However, this is a relatively niche customer base. The biggest growth in adoption would come from making this product potentially appeal to everyone by adding the element of touch to some forms of digital entertainment. However, this concept would need to be developed much further past the scope of a single capstone term before it could even have the potential break into being any kind of mainstream entertainment device.

## Installation and Flashing
### Arduino IDE
To set up the Arduino IDE for the Vibrosonics project, follow these steps:
1. Install the Arduino IDE for your operating system.
2. Install the ESP32 board to the Arduino IDE:
    - Navigate to `Tools` > `Board` > `Boards Manager`.
    - Search for `ESP32` and install the esp32 board library by Expressif.
3. In the top left, click the dropdown and choose `Adafruit ESP32 Feather` for board and `COMX` for port where `X` is a number 0-6.
    - Board needs to be plugged in for this to work.
    - May need to download drivers in order to detect the port.
4. Next, download the zip files for Vibrosonics, AudioLab, AudioPrsim, and Fast4ier
5. Once all of the zip files are finished downloading, go back to the Arduino IDE.
6. Navigate to `Sketch` > `Include Library` > `Add .ZIP library`
    - Do this for each library.

You should be ready to use the example sketches or create your own! To upload a
sketch to the Vibrosonics hardware:
1. Ensure the ESP32 microcontroller in the Vibrosonics hardware is plugged in
   via USB to the computer you want to upload your sketch from.
2. To upload your sketch from the Arduino IDE, go to the top drop down menu and
   click `Select other board and port...` 
3. In the pop up window, search for `Adafruit ESP32 Feather` and select it
   along with the port listed.
4. Once that is finished, you can click the arrow in the top left of the
   Arduino IDE to compile and upload your sketch to the Vibrosonics hardware.

## Library Architecture
The Vibrosonics library aims to streamline three processes to enable
translating audio into the haptic range: audio signal processing, audio
analysis and finally audio synthesis. We utilize a few libraries to achieve
this. The first is AudioLab, which helps in signal processing and additive
synthesis. The second is Fast4ierwhich is used to perform the Fast Fourier
Transform (FFT). Finally, we use the AudioPrism library to analyze the
frequency spectrum obtained from the FFT result.

We have created an API, \ref VibrosonicsAPI, that combines these libraries to
provide a simple interface for audio processing, analysis and synthesis.

Additionally, there is the \ref Grain class which provides a way to implement
granular synthesis, allowing for sound wave shaping through envelopes. There
are the \ref GrainList and \ref GrainNode classes that help you manage a
linked-list of grains.

## Special thanks to:
 - Dr. Chet Udell (Project leader and manager)
 - Vincent Vaughn (Advised software and hardware development)
 - Alex Synytsia (Participated in hardware development for the project during the 2022-2023 years)
 - Nick Synytsia (Developed AudioLab and advised software development for 2024-2025)
