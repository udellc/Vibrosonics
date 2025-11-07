# Developer Setup

## Table of Contents

- [Installation (Arduino IDE)](#installation-arduino-ide)
- [Web Development Setup](#web-development-setup)

## Installation (Arduino IDE)

### 1. Install Arduino IDE and ESP32 Board Support

- Download [Arduino IDE](https://www.arduino.cc/en/software/)
- Go to **Tools > Boards > Boards Manager**
- Search and install `esp32` by **Espressif Systems**

![Installing the board manager](/docs/assets/images/Board_library.png)

### 2. Set Board and Ports

- Plug in the board (you may need to install USB drivers)
- Go to the top drop down menu and click `Select other board and port...`
- In the pop up window, search for `Adafruit ESP32 Feather` and select it along
with `COMX` for port where `X` is a number 0-6
  - You may need to download a driver to see the port ([Windows
    Tutorial](https://randomnerdtutorials.com/install-esp32-esp8266-usb-drivers-cp210x-windows/))
  - On Linux, this is the `ch341` driver

![The other board and port drop down](/docs/assets/images/Confirm_board.png)

### 3. Add Libraries

Navigate to the Arduino libraries folder. This is usually located in
`Documents/Arduino/libraries/` for Windows and `~/Arduino/libraries/` for Mac and Linux.

Clone the repositories into the libraries folder:

```bash
cd "/path/to/Arduino/libraries/"
git clone https://github.com/synytsim/AudioLab.git
git clone https://github.com/udellc/AudioPrism.git
git clone https://github.com/jmerc77/Fast4ier.git
```

Verify that the libraries are installed by opening Arduino IDE and going to
**Sketch > Include Library > Manage Libraries...**. You should see `AudioLab`,
`AudioPrism`, and `Fast4ier` in the list.

### 4. Upload a Sketch

You should be ready to use the example sketches or create your own! To upload a
sketch to the Vibrosonics hardware:

- Plug in the ESP32 via USB
- Open your own sketch or one of our examples from **File > Examples >
Vibrosonics > \[Example Name]**
- Press the upload (➜) in the top left to compile and upload your sketch

![Uploading a sketch to the arduino](/docs/assets/images/Upload_sketch.png)

## Web Development Setup

### 1. Install Node.js and npm

- Download and install Node.js through your prefered web browser. This will also install npm.

### 2. Install Dependencies

- Open a terminal and navigate to the `WebApp/` directory of the repository.

- Run the following command to install the required dependencies:

```bash
npm install
```

### 3. Start the Development Server

- Run the following command to start the development server:

```bash
# This will start the server and provide a local URL at localhost. Once the server is running,
npm run dev

# Opens the application in your default web browser.
o

# Manually restarts the server
r

# Stops the server
q
```

### 4. Additional Commands

**Production Build:**
To build the application for production, run:

```bash
# This will produce optimized files in the `dist/` directory
npm run build

# This will preview the production build locally
npm run preview
```

**Run Linter Tool:**
To run ESLint for .js and .jsx files, run:

```bash
# This will scan the target files for any syntax issues or potential bugs
npm run lint

# This will automatically fix any fixable issues found by ESLint
npm run lint.fix
```

### 5. Troubleshooting

If you encounter any issues during setup or development, consider the following steps:

- Ensure that Node.js and npm are correctly installed by running `node -v` and `npm -v` in your terminal.
- Check for any error messages in the terminal and refer to the documentation of the specific tools or libraries being used.
- Make sure all dependencies are installed correctly by running `npm ci` for a clean install. If issues still persist, try deleting the `node_modules/` directory and running `npm install` again.
