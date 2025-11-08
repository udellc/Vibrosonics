# Developer Setup

## Table of Contents

- [Installation (Arduino IDE)](#installation-arduino-ide)
- [Web Development Setup](#web-development-setup)
- [ESP32 Web Server Setup](#esp32-web-server-setup)
- [Uploading Web App Build into ESP32](#uploading-web-app-build-into-esp32)

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

### 6. Install Web Server Libraries for the ArduinoIDE

**ArduinoIDE Library Manager:**
To make requests for the web server on the ESP32, install the following libraries,

![ESP Async WebServer Library](/docs/assets/images/ESP_web_server_library.png)

![Async TCP Library](/docs/assets/images/ESP_async_tcp_library.png)

## ESP32 Web Server Setup

**Little File System Setup:**
To deploy the WiFi web app onto the ESP32, we'll need to store it into non-voltile memory. We use Little FS to store the web app build files, to setup the file system plugin, consider the following,

### 1. Download the Little FS VSIX File

- Navigate to [Little FS Git Repository](https://github.com/earlephilhower/arduino-littlefs-upload/releases) and download the .vsix file. As of 11/06/2025, we are using Release version 1.6.0.

![Little FS VSIX file](/docs/assets/images/Little_FS_file.png)

### 2. Move the VSIX File to Plugins Directory

- Move the VSIX file into the **.arduinoIDE > plugins** directory. Create the directory if necessary.

![Little FS Directory](/docs/assets/images/Little_FS_file_directory.png)

### 3. Verify Little FS Plugin in ArduinoIDE

- Restart the ArduinoIDE and enter **[Ctrl] + [Shift] + [p]** to open the list of commands and look verify that **Upload LittleFS to Pico/ESP8266/ESP32** exists.

![Little FS Command](/docs/assets/images/Little_FS_command.png)

## Uploading Web App Build into ESP32

**Steps:**
To upload the Preact web app into the ESP32, we'll need to build the web app and move them into a directory LittleFS knows about.

### 1. Build Web App

```bash
# CD into the web app directory
cd WebApp

# Build production level web app
npm run build

# Create a data directory under src/main
cd ../src/main
mkdir data
```

### 2. Upload Build Output into ESP32

- Now move or copy the output of the web app build directory into the data directory. The output directory should be named `dist` or `build`. Make sure to not include the dist directory.
- Open ArduinoIDE and use the Little FS upload command. Make sure that the ESP32 is connected and the Serial Monitor is closed.

### 3. Compile and Verify

- Once the files are uploaded, compile and upload the `main.ino` file using the AruduinoIDE with your network's SSID and password and open the Serial Monitor.
- Once the ESP32 connects to your network, the Serial Monitor should have print an IP link that you can open using a web browser.
- Confirm that the web app mirrors the development web app.
