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

| Component      | ESP32-Cam Pin |
| -------------- | ------------- |
| PIR Sensor     | GPIO 12       |
| MicroSD CS     | GPIO 5        |
| LED (optional) | GPIO 4        |

## Flashing from Linux

The ESP32-CAM has no USB port. It is flashed over its serial pins using a
USB-to-serial adapter — here an SH-U09C (FT232RL). The ESP32-CAM-MB shim does
the same job but covers every pin, so it is not used once the unit is wired up.

### Wiring

**Set the adapter's voltage jumper to 3.3 V.** The ESP32's pins are not 5 V
tolerant, and at 3.3 V the adapter cannot supply the ~300 mA the board needs
anyway — so power comes from elsewhere and the adapter only carries signals.

| SH-U09C | ESP32-CAM | Notes |
| --- | --- | --- |
| TXD | U0R (GPIO 3) | adapter transmit → board receive |
| RXD | U0T (GPIO 1) | adapter receive → board transmit |
| GND | GND | |
| *VCC — leave unconnected* | | see below |

Power separately, into the **5 V** pin from a 5 V supply that can hold up under
load (a phone charger, or an LM2596 module from a 12 V brick). Tie its ground
to the adapter's ground so both share a reference.

To enter the bootloader, **connect GPIO 0 to GND**, then power-cycle or press
reset. Remove that jumper and reset again to run normally.

### One-time Ubuntu setup

```bash
# Serial port access, then log out and back in for it to take effect
sudo usermod -aG dialout $USER

# Only if /dev/ttyUSB0 fails to appear: Ubuntu's braille daemon can grab FTDI
# adapters. Check first -- it is not installed on every system.
dpkg -l | grep brltty && sudo apt remove brltty

# PlatformIO
source ~/.global_venv/bin/activate
pip install platformio
```

Confirm the adapter is detected — it should appear as `/dev/ttyUSB0`:

```bash
ls -l /dev/ttyUSB*
```

### Build, flash, monitor

```bash
source ~/.global_venv/bin/activate

pio run                      # compile only
pio run -t upload            # compile and flash
pio device monitor           # serial console at 115200

# If more than one serial device is present:
pio run -t upload --upload-port /dev/ttyUSB0
```

Flashing sequence: jumper GPIO 0 to GND → reset → run `pio run -t upload` →
remove the jumper → reset.

`monitor_filters = esp32_exception_decoder` is enabled, so a crash prints a
decoded stack trace with real function names instead of raw addresses.

### Arduino IDE

`ArduinoIDE-ESP32-Cam_Security/` is a flattened mirror of the same code for the
Arduino IDE. Select board **AI Thinker ESP32-CAM** and partition scheme **Huge
APP**. Note that this copy is generated — see `CLAUDE.md` before editing it.

## Motion sensor wiring — the inverter is required

The sensor does **not** connect straight to GPIO 13. It drives an NPN inverter
first:

```
   SENSOR OUT ──[ 10 kΩ ]──┐
                           │ B
                        C  ├─────╲
   GPIO 13 ────────────────┤      │  2N2222
                           ├─────╱
                           │ E
   GND ────────────────────┘
```

| Transistor pin | Connects to |
| --- | --- |
| Base | sensor `OUT`, through 10 kΩ (4.7 k–22 k all work) |
| Collector | `GPIO13` |
| Emitter | `GND` |

**Why this is not optional.** GPIO 13 is also **SD_MMC D3**. A card samples D3
while initialising and comes up in SPI mode if it is held LOW, after which
`SD_MMC.begin()` fails with a `0x107` timeout. An HC-SR501 actively drives its
output LOW while idle, which broke the card mount on roughly two thirds of
boots — intermittently, because it happened to work whenever the sensor was
still asserted from a recent trigger.

The inverter makes the pin idle **HIGH** (what D3 needs) and pull **LOW** on
motion. `MOTION_ACTIVE_LEVEL` in `ConfigSettings.h` matches this and is what
`ext0` is armed against.

TO-92 transistor pinouts vary between E-B-C and C-B-E. If the marking is
unclear, find the base with a meter's diode-test mode: it reads ~0.7 V to both
other pins in one polarity.

## Production wiring (battery build)

Moving off the bench supply. The goal is a build that stays serviceable with
dupont connectors, keeping soldering to the two places it genuinely earns its
place.

### The power chain

```
4x 18650 (PARALLEL, 3.7 V) → fuse → buck-boost module → ESP32-CAM 5V/GND
                                          │
                                    1000 µF + 100 nF
```

**Verify the holder is parallel before anything else.** With cells installed it
must read **3.7–4.2 V**. If it reads ~15 V it is wired in series, and connecting
that to a TPS63020-class module (5.5 V input maximum) destroys it instantly.

**Fit an inline fuse** on the pack's positive lead — 2–3 A, a cheap automotive
blade or glass fuse. Four parallel 18650s can deliver tens of amps into a short,
and inside a sealed plastic box on a wall that is the one failure worth
engineering against. This is the single most important part of the build.

### Where the transistor and resistor live

**Use a 170-point mini breadboard.** It is roughly 47 x 35 x 9 mm, fits the
housing comfortably, needs **no soldering at all**, and keeps every connection
serviceable — which is the same reasoning behind choosing dupont over solder.

Lay it out exactly as the bench rig, since that arrangement is already proven:

| Column | Holds | And |
| --- | --- | --- |
| sensor | wire from HC-SR501 `OUT` | 10 kΩ resistor, leg 1 |
| E | transistor emitter | wire to `− rail` |
| B | transistor base (middle leg) | 10 kΩ resistor, leg 2 |
| C | transistor collector | wire to `GPIO13` |

The mini board's two long rails carry 5 V and GND, so the capacitors, the
sensor's `VCC`/`GND`, and the ESP32's `5V`/`GND` all land there too.

A dab of hot glue on the transistor and capacitors stops vibration walking them
out of their holes, without making anything permanent.

### What actually needs soldering

Only two joints, and both are at the battery end:

1. **Pack leads to the buck-boost input.** Most modules have solder pads rather
   than screw terminals. If yours has terminals, you are done with no iron at
   all.
2. **A polarised connector between sled and body**, so the battery sled can be
   pulled without unplugging anything by hand. XT30 or JST-XH; crimping avoids
   soldering if you have the tool.

Everything else — sensor, transistor, resistor, capacitors, ESP32 — is dupont
into the mini breadboard.

### Keep a reflash path

Bring `U0R`, `U0T`, `GND` and `IO0` to four spare rows near the microSD access
door. Without that, changing a setting means dismantling the housing. Flashing
also needs `IO0` jumpered to `GND`, so leave those two rows adjacent.

### Order of assembly

Build and test in this order so a fault is always in the thing you just added:

1. Bench supply → confirm the board boots and records
2. Add the sensor and inverter → confirm a motion wake
3. Swap the bench supply for buck-boost + fuse, **cells not yet connected**
4. Measure the converter output — it must read 5.0 V (or 3.3 V if you are
   feeding the 3V3 pin) **before** it touches the ESP32
5. Connect the pack and run a full cycle

### Before every power-up

Meter on continuity between the 5 V and GND rails. Silent means no short. It
takes fifteen seconds and catches essentially every wiring mistake that matters.

## First run

There is no "run" command — this is firmware. Once flashed it starts on its own
every time the board powers up, and keeps running until power is removed. What
you do is flash it, then *watch* it.

**1. Prepare the SD card.** Format FAT32 and put a `config.txt` in the root:

```
WIFI_SSID=your_wifi_name
WIFI_PASSWORD=your_wifi_password
SERVER_URL=http://your-server:8000/upload
CAMERA_ID=camera1
```

Without this the board logs `config.txt missing or incomplete` and reboots in a
loop. Uploads are HTTP `PUT` to `SERVER_URL/CAMERA_ID/filename.avi`.

**2. For bench testing, disable deep sleep.** In `include/ConfigSettings.h`:

```cpp
constexpr bool ENABLE_DEEP_SLEEP = false;
```

Otherwise the board records once and sleeps, which is correct behaviour but
awkward to iterate against. Re-enable it before doing any battery measurements.

**3. Flash and open the console:**

```bash
source ~/.global_venv/bin/activate
pio run -t upload      # with GPIO 0 jumpered to GND, then reset
                       # remove the jumper and reset
pio device monitor
```

**4. Trigger a recording.** You don't need the motion sensor connected — just
**briefly touch a jumper from GPIO 13 to 3.3 V**. That's exactly what the sensor
does. Expect roughly:

```
Boot 1 (power-on / reset)
SD Card Type: SDHC
Config loaded: ...
Camera ready (JPEG capture mode)
WiFi Connected!  IP Address: 192.168.x.x
Motion detected! Recording video...
Recorded 30 frames, 3.0 seconds, avg FPS: 10.0 (target: 10)
Recording complete: 300 frames in 30.0 seconds (avg 10.0 FPS)
AVI file finalized
Uploading file: /rec_000001_00.avi (Size: ...)
Upload successful
```

**5. Verify the clip.** Pull the SD card (or grab the uploaded file) and check
it properly — playing it in VLC is not sufficient, since VLC recovers from
malformed containers:

```bash
ffprobe -v error -select_streams v:0 \
  -show_entries stream=nb_frames,r_frame_rate -show_entries format=duration \
  -of default=nw=1 rec_000001_00.avi
ffmpeg -v error -i rec_000001_00.avi -f null -    # any output = corruption
```

Expect `nb_frames=300`, `r_frame_rate=10/1`, `duration=30.0`, and silence from
the second command.

### Troubleshooting the first run

| Symptom | Likely cause |
| --- | --- |
| Board resets repeatedly under load | Supply can't hold up during WiFi TX. Add bulk capacitance at the 5 V pin; don't mask it with `DISABLE_BROWNOUT_DETECTOR` |
| `Camera init failed with error 0x105` | Power, or camera ribbon not seated |
| Frame rate well under target | Try `XCLK_FREQ_HZ = 10000000`; also suspect a slow SD card |
| `Could not open /dev/ttyUSB0 ... No such file` | **The port number moved.** Every replug can bump it (`ttyUSB0` → `ttyUSB1` → ...). Run `ls /dev/ttyUSB*` to see the real one, or use the stable `/dev/serial/by-id/...` path, which never changes |
| No `/dev/ttyUSB*` at all | `brltty` installed (check first — it isn't on every system), or missing `dialout` group |
| Garbage on the serial console | Baud mismatch — monitor must be 115200 |

### Testing the AVI container without hardware

The container layout is validated on the host with `g++` and `ffprobe`, no
ESP32 required. See `test/README`.

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
