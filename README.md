# Vibrosonics
Welcome to Vibrosonics!

Detailed documentation can be viewed under the [wiki](https://github.com/udellc/Vibrosonics/wiki)

Hardware: Adafruit ESP32 Feather, MAX9744 Amplifier board, TT25-8 puck transducer, 3.5mm audio jack cable

Library dependencies:
 - For Vibrosonics-Mozzi: Arduino, ArduinoFFT, Mozzi.
  
 - For Vibrosonics-Interrupt: Arduino, ArduinoFFT 
  
 - NOTE: In ArduinoFFT library, all instances of 'double' must be converted to 'float' in source code files. This is done for a performance boost. Soon the library with the changes described will be pushed onto this repository for 'plug and play' operation.
