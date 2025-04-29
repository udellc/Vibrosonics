For those who utilize this branch in the future:

This ece_dev branch was created for the 2024-2025 Vibrosonics capstone cycle. During this cycle, the CS and ECE teams worked on separate goals in software and hardware. As such, the CS team's code did not include implementation for the newly implemented hardware. This "operating code" has little relation to the primary Vibrosonics codebase; instead, it is a fairly primitive version of Vibrosonics which is meant soley to demonstrate the operating of new hardware.

In particular, this branch contains code which supports:
- 4 channel DAC output via an external AD5644 digital-analog converter
- FM radio module initialization

For the most part, this code will not be particularly helpful for future CS students, as the signal processing in the proper Vibrosonics codebase is far superior. The thing that you likely need to reference this for is the implementation of the external quad-DAC. The existing codebase has some snippets about the MCP-4728, but we phased that DAC out in favor of the AD5644. 

View vibrosonics.cpp, especially the speakerMode() and initialize() functions, to see how to initialize the DAC and the SPI communication that allows it to work with the board. 

Additionally, AD56X4_alt.cpp is a modified version of a library originally created by Feja Nordsiek. I modified it to optimize the DAC's channel writes to be less generalized and instead more specific to Vibrosonics. This optimization is significant (22us per channel write) and I would highly recommend using it over the original code; it will give you much more flexibility within our limited interrupt cycle time. (122us at 8kHz)

Here is the original AD56X4 library GitHub page: https://github.com/frejanordsiek/arduino_library_AD56X4

---------------------------------------------------

To those who work on this project in the future, I wish you the best.

Fiona Pendergraft 

[Updated April 29th, 2025]
