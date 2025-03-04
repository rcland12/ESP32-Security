# ESP32-Cam Security System

A motion-activated security camera system built with ESP32-Cam that records video when motion is detected and uploads footage to a server.

## Overview

This project uses an ESP32-Cam microcontroller with a PIR motion sensor to create a security camera system. When motion is detected, the system records video to an SD card and then uploads it to a server for remote storage and viewing.

## Features

- Motion detection using PIR sensor
- Automatic video recording (configurable length and frame rate)
- Local storage to microSD card
- Automatic upload to remote server
- WiFi connectivity
- Configurable via a settings file on the SD card

## Hardware Requirements

- ESP32-Cam board
- PIR motion sensor
- MicroSD card
- 5V power supply
- Breadboard and jumper wires for prototyping

## Pin Connections

| Component | ESP32-Cam Pin |
|-----------|---------------|
| PIR Sensor | GPIO 12 |
| MicroSD CS | GPIO 5 |
| LED (optional) | GPIO 4 |

## Installation

1. Clone this repository
2. Open the project in PlatformIO
3. Connect your ESP32-Cam to your computer
4. Build and upload the code

## Project Structure

```
├── include/
│   └── PinDefinitions.h       # Pin definitions and constants
├── lib/
│   ├── ConfigManager/         # Configuration management library
│   │   └── src/
│   │       ├── ConfigManager.cpp
│   │       └── ConfigManager.h
│   ├── NetworkManager/        # WiFi and server connection management
│   │   └── src/
│   │       ├── NetworkManager.cpp
│   │       └── NetworkManager.h
│   ├── SDCardManager/         # SD card operations
│   │   └── src/
│   │       ├── SDCardManager.cpp
│   │       └── SDCardManager.h
│   └── VideoRecorder/         # Camera and video recording operations
│       └── src/
│           ├── VideoRecorder.cpp
│           └── VideoRecorder.h
└── src/
    └── main.cpp               # Main application code
```

## Configuration

Create a `config.txt` file on the SD card with the following format:

```
WIFI_SSID=your_wifi_name
WIFI_PASSWORD=your_wifi_password
SERVER_URL=http://your-server-url.com/upload
CAMERA_ID=camera1
```

## Usage

1. Insert a formatted microSD card with the config.txt file
2. Power on the ESP32-Cam
3. The system will initialize and start monitoring for motion
4. When motion is detected, it will record video for the configured duration
5. After recording, the video is automatically uploaded to the server
6. If upload is successful, the local file is deleted to save space

## Customization

You can modify the following parameters in `PinDefinitions.h`:

- `PIN_PIR`: GPIO pin for the PIR sensor
- `FRAME_SIZE`: Resolution of the video (FRAMESIZE_VGA, FRAMESIZE_SVGA, etc.)
- `JPEG_QUALITY`: JPEG quality (0-63, lower is higher quality)
- `FRAME_RATE`: Video frame rate
- `RECORD_TIME`: Duration of recordings in seconds

## Troubleshooting

- **SD Card Mount Failed**: Ensure the SD card is properly formatted (FAT32) and inserted correctly
- **Camera Init Failed**: Check camera connections and power supply
- **WiFi Connection Failed**: Verify WiFi credentials in config.txt
- **Upload Failed**: Check server URL and internet connectivity

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- ESP32 Camera library
- Arduino framework
- PlatformIO development environment