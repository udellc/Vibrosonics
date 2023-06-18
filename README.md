# Vibrosonics
Welcome to Vibrosonics!

Detailed documentation can be viewed under the [wiki](https://github.com/udellc/Vibrosonics/wiki).

Hardware: Adafruit ESP32 Feather, MAX9744 Amplifier board, TT25-8 puck transducer, 3.5mm audio jack cable

Library dependencies:
 - For Vibrosonics-Mozzi: Arduino, ArduinoFFT [v1.5.6](https://github.com/kosme/arduinoFFT/releases/tag/v.1.5.6), Mozzi [v1.0.3rc6](https://github.com/sensorium/Mozzi/releases/tag/v1.0.3rc6)
  
 - For Vibrosonics-Interrupt: Arduino, ArduinoFFT [v1.5.6](https://github.com/kosme/arduinoFFT/releases/tag/v.1.5.6)
  
 - NOTE: In ArduinoFFT library, all instances of 'double' must be converted to 'float' in source code files. This is done for a performance boost. Soon the library with the changes described will be pushed onto this repository for 'plug and play' operation.

 - On versions: There is no apparent reason to believe that the most recent versions (as of writing) of ArduinoFFT (v1.6) and Mozzi (v1.1.0) wouldn't work given what they change from the prior versions, but they just haven't been tested since their release. Some light experimentation with the newest versions to see if they work the same is optional.

## What is it?

The goal of the Vibrosonics project is to enable those who struggle with or cannot hear to enjoy audio-based entertainment in a different way as well as potentially enhance audio-related experiences for enthusiasts by adding the sensation of touch. 

To that end, the aim is to create an easily wearable portable device than can take in audio signals and convert them to vibrations reflective of the intensity and pitch of the initial sound.

The majority of initial adoption would likely be from users for whom the product directly fills an unmet need. Within the context of this device, this will be clients who require haptic feedback as a replacement for audio. Examples may include those who are deaf or hard of hearing, or employers whose work environments make pure audio based communication difficult and require alternatives. Subsequent adopters are likely to be enthusiasts who will use the product to enhance events or activities they already partake in by adding a tactile aspect, such as in silent raves or virtual and alternate reality gaming. 

Currently, the market for similar devices is highly limited and undeveloped. This project seeks to expound upon what little exists by incorporating a higher range of translated audio frequencies along with more detailed audio-to-haptic translation into the final tactile experience. Ideally, this product would progressively gain new users by becoming a serious consideration for the aforementioned groups that require an alternative to pure audio. However, this is a relatively niche customer base. The biggest growth in adoption would come from making this product potentially appeal to everyone by adding the element of touch to some forms of digital entertainment. However, this concept would need to be developed much further past the scope of a single capstone term before it could even have the potential break into being any kind of mainstream entertainment device.

## Special thanks to:
 - Dr. Chet Udell (Project leader and manager)
 - Vincent Vaughn (Advised software development)
 - Alex Synytsia (Participated in hardware development for the project during the 2022-2023 years)
