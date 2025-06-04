## VibroSonics ECE Team Operating Code

For those who utilize this branch in the future:

This ece_dev branch was created for the 2024-2025 VibroSonics capstone cycle. During this cycle, the CS and ECE teams worked on separate goals in software and hardware. As such, the CS team's code did not include implementation for the newly implemented hardware. This "operating code" has little relation to the primary VibroSonics codebase; instead, it is a fairly primitive version of VibroSonics which is meant soley to demonstrate the operating of new hardware.

For the most part, this code will not be particularly helpful for future CS students, as the signal processing in the proper VibroSonics codebase is far superior. The aspect of this that you will likely need to reference is the implementation of the external quad-DAC. The existing codebase has some snippets about the MCP-4728, but we phased that DAC out in favor of the AD5644. 

View ``vibrosonics.cpp``, especially the ``speakerMode()`` and ``initialize()`` functions, to see how to initialize the DAC and the SPI communication that allows it to work with the board. Additionally, read the Guide to AD5644 Implementation below.

Additionally, ``AD56X4_vibrosonics.cpp`` is a modified version of a [library originally created by Feja Nordsiek](https://github.com/frejanordsiek/arduino_library_AD56X4). I modified it to optimize the DAC's channel writes to be less generalized and instead more specific to VibroSonics. This optimization is significant (~22μs per channel write down from ~35μs) and I would highly recommend using it over the original code; it will give you much more flexibility within our limited interrupt cycle time, which is ~122μs at 8kHz.

## Guide to AD56X4 Implementation

The AD5644 external quad-DAC allows us to achieve four-channel output from our ESP32 Feather. This is vital because it allows us to support stereo output; we can assign two exciters with individualized signals to both our mids and highs frequency ranges. There is work to be done during further cycles to integrate the AD5644 into the existing VibroSonics software, as well as some important considerations when continuing the hardware implementation with the AD5644 in mind. This section hopes to make the process of working with the AD5644 much clearer.

### <ins>AD5624/AD5644/AD5664: What's the difference?</ins>

To begin, it is important to note that the AD5644 is actually one of a three-part series of external quad DACs alongisde the AD5624 and AD5664. The only difference is the range of bits that the analog outputs use; the AD5624/AD5644/AD5664 use 12, 14, and 16 bits respectively. For this reason, the code pertaining to the DAC refers to it as the AD56X4. 

The choice to use the AD5644 specifically was largely arbitrary, and to my understanding any of the three should be suitable for the VibroSonics project. The primary place I encountered where output size is relevant is when trying to generate signals with maximum amplitude; the ampltiude of the sinusoidal wave should be ``2^(AD56X4_SIZE - 1)``, where ``AD56X4_SIZE`` is the number of bits. ``vibrosonics.h`` contains a global constant which can be modified to be 12, 14, or 16 as necessary. 

### <ins>AD56X4 Pinouts</ins>

![image](https://i.postimg.cc/Pf4CFPNQ/image.png)

The [datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ad5624r_5644r_5664r.pdf) for the AD56X4 series goes into more detail, but here is a pinout rundown as it pertains to the VibroSonics project:

[1] V<sub>OUT</sub>A - Channel A analog output. 12, 14, or 16 bits depending on AD56X4 model.

[2] V<sub>OUT</sub>B - Channel B analog output. 12, 14, or 16 bits depending on AD56X4 model.     

[3] GND - Ground reference pin. This should be tied to ESP32 Feather's ground line.

[4] V<sub>OUT</sub>C - Channel C analog output. 12, 14, or 16 bits depending on AD56X4 model.

[5] V<sub>OUT</sub>D - Channel D analog output. 12, 14, or 16 bits depending on AD56X4 model.  

[6] SYNC - Active low control input. This is where we should connect our slave select pin. For the purposes of this library, I chose to use the ESP32 Feather's Pin 33. This can be modified in ``vibrosonics.h`` if needed.

[7] SCLK - Serial clock input. This should be tied to the ESP32's SCK pin.

[8] DIN - Serial data input. This should be tied to the ESP32's MOSI pin.

[9] V<sub>DD</sub> - Power supply input. This can receive anywhere from 2.7V to 5.5V; I recommend the ESP32 Feather's 3.3V line. Note that this should be decoupled with 10μF and 0.1μF capacitors in parallel.

[10] V<sub>REFIN</sub>/V<sub>REFOUT</sub> - When using the AD56X4's internal reference, which I recommend, this can be left floating. **Note that you MUST specify that you are using internal reference in your initialization code for the DAC:**

```c++
dac.useInternalReference(SS_PIN, true);
```
To summarize:
- Our four analog channels should output to any signal post-processing circuitry is necessary before the signal reaches the board's output connectors.
- Our V<sub>DD</sub> and GND should connect to the ESP32 Feather's 3.3V and GND lines, with the consideration that V<sub>DD</sub> should be decoupled with 10μF and 0.1μF capacitors in parallel.
- SYNC, SCLK, and DIN should be connec tto the ESP32 Feather's Pin 33, SCK, and MOSI pins respectively.
- V<sub>REFIN</sub>/V<sub>REFOUT</sub> should be left floating, but use of the internal reference must be specified in the code.

### <ins>Initializing the AD56X4 DAC</ins>

Here is the code in ``vibrosonics.cpp`` for writing to the DAC:

```c++
  Serial.println("Initializing SPI communication...");
  pinMode(SS_PIN, OUTPUT); // Setting slave select pin to output mode
  SPI.begin(); // Begins SPI communication

  Serial.println("Initializing AD56X4 DAC...");
  dac.reset(SS_PIN, true); // This is necessary! As outlined in AD56X4_vibrosonics.cpp, some important function calls were moved here for optimization purposes.
  dac.useInternalReference(SS_PIN, true); // Very necessary if leaving VREFOUT/VREFIN pin open on the AD56X4.
```
As we can see, we need to:
- Set our Slave Select pin to output mode
- Begin SPI communication
- Run our AD56X4 reset() function
- Run our AD56X4 useInternalReference() function

Once these four requirements are met, we are ready to write data to the DAC.

### <ins>Writing to the AD56X4 DAC</ins>

The AD56X4 function for writing to a channel follows this syntax:

```c++
dac.setChannel(int ss_pin, byte mode, byte channel, word value); 
```
We provide the following parameters:
- int ss_pin: We provide our slave select pin, which is Pin 33 for our usecase, otherwise defined as SS_PIN in this library
- byte mode: The write mode. For our purposes, we want to use AD56X4_SETMODE_INPUT_DAC, which immediately outputs the value rather than just populating the buffer.
- byte channel: There are four bytes corresponding to each of the four channels: AD56X4_CHANNEL_A, AD56X4_CHANNEL_B, AD56X4_CHANNEL_C, and AD56X4_CHANNEL_D.
- word value: Typically, it is best to take an integer value and type-cast it to a word.

Here is a practical example of this function being used, from FFTMode():

```c++
dac.setChannel(SS_PIN, AD56X4_SETMODE_INPUT_DAC, channel[current_channel], (word)(value));
```
Note that the channel[] array is simply a 4-wide list of the four channel bytes, so that they can be referred to with their indices 0, 1, 2, and 3 instead.

### <ins>Timing Considerations</ins>

As mentioned before, this modified version of the AD56X4 library successfully reduces the time to write to a channel from 35μs to 22μs. Why is this significant?

At our target sampling frequency of 8kHz, we only have 122 μs between interrupts. Because 46μs is used for the analogRead() we perform each sample cycle, we have 76μs to perform all other actions. Note that the actions taken during the interrupt need to leave a significant amount of extra time so that non-interrupt actions have time to run. 

Because we only need to output frequencies in the haptic range, we can get away with only writing to one channel every sample cycle. This gives an effective sample rate per channel of 2kHz (8kHz / 4). The resulting Nyquist frequency is then 1kHz, plenty high for our purposes. 

Using one read and one optimized write, we end up with a per-sample cycle read/write time of 68μs, leaving us with 54μs to spare. If higher frequencies are needed, one could slip in a second 22μs write and still have 32μs left over. Without optimization, two writes would be impossible as it would leave a measly 6μs.

---------------------------------------------------

To those who work on this project in the future, I wish you the best.

Fiona Pendergraft 

[Updated June 3rd, 2025]
