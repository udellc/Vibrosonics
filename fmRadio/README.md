# FM Radio Prototype Handoff
This is the confirmed stable state of the FM Radio implementation at the end of development for VibroSonics 25-26.

## Contents
1. FM Radio Schematic
Contains the hardware chematic showing connections between:
- ESP32 Feather HUZZAH
- Si4703 FM Tuner
- Audio Output circuitry
THe schematic should be referenced when rebuilding or troubleshooting the hardware setup.

2. SparkFun_Si4703 Library
Contains the Arduino library required to communicate with the Si4703 tuner.
- Install the library before compiling the project

## Hardware Components
### Required Hardware
- ESP32 Feather HUZZAH
- SparkFun Si4703 FM Tuner Breakout
- Audio output device with 3.3mm jack (required for FM reception)
- Breadboard, jumper cables, 10k resistors
- USB cable for ESP32 programming

### Hardware Notes
- The audio device cable with 3.3mm jack acts as the FM antenna
- Audio quality may degrade if antenna connection is poor
- Noise is expected if building with breadboard


### Pin Connections
| Si4703 Pin     | ESP32 Pin     |
| :---           | :---          |
| SDA            | GPIO 23       |
| SCL            | GPIO 22       |
| RST            | GPIO 27       |
| GPIO 2         | GPIO 4        |

Verify pin assignments against the schematic before powering the system.
<img src="reference images/schematic.png>" alt="Schematic" width="500">

## Software Requirements
### Development Environment
- Arduino IDE 2.x
- ESP32 Board Support Package
- SparkFun Si4703 Library

### Installation
1. Install Arduino IDE
2. Install ESP32 board definitions
3. Import the SparkFun Si4703 library
4. Open the FM Radio sketch 
    - located in ```SparkFun_Si4703\examples\Si4703_Radio_Test\sketch_may14a```
5. Select the correct ESP32 board and COM port
6. Upload the sketch

## Startup Procedure
1. Connect the Si4703 tuner to the ESP32 according to the schematic
2. Connect headphones or speaker through AUX port on Si4703
3. Power the ESP32 through USB
4. Upload the radio firmware
5. Open the Serial Monitor
6. Verify that the tuner initializes successfully

## Known Limitations
### Static Audio
Some static is expected if:
- The antenna is too short
- The selected station has weak reception
- Connected to a breadboard that produces noise

## Troubleshooting
### Tuner Not Detected through I2C
- Verify SDA and SCL wiring
- Confirm the ESP32 is supplying power
- Check serial output for initialization errors
- Ensure 10k pull-up resistors are connected to SCL and SDA lines 