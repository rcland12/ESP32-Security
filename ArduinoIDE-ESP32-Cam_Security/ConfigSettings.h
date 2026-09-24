#ifndef CONFIG_SETTINGS_H
#define CONFIG_SETTINGS_H

#include "esp_camera.h"
#include <stdint.h>

constexpr int RECORD_TIME_SEC =               30;
constexpr int FRAME_RATE =                    10;
constexpr framesize_t FRAME_SIZE = FRAMESIZE_VGA;
constexpr int JPEG_QUALITY =                  10;

// Sensor pixel clock. 20 MHz is the OV2640's nominal rate, but it produced zero
// captures on this hardware -- esp_camera_init() succeeded and every
// esp_camera_fb_get() then returned NULL. 10 MHz is the known-good value this
// project has always used. Raise it again only with a capture test to prove it.
constexpr int XCLK_FREQ_HZ =            10000000;

// SD writes are flushed on this cadence rather than per frame. Flushing every
// frame forces a FAT metadata update per frame and throttles the frame rate.
constexpr uint32_t FLUSH_INTERVAL_FRAMES =     30;

// Slack over the nominal frame count, for when capture runs slightly ahead.
constexpr uint32_t FRAME_COUNT_HEADROOM =      20;

// Pause after Serial.begin() so a monitor attached by hand catches the banner.
// This is pure awake time on every wake, so keep it short in production; raise
// it to 1000 while actively debugging.
constexpr uint32_t SERIAL_SETTLE_MS =         150;

// Attempts (x500 ms) allowed for a cached-channel reconnect before falling back
// to a full scan. Short, because the fast path either works quickly or not.
constexpr int WIFI_FAST_ATTEMPTS =             12;

// Delay before rebooting after an unrecoverable setup failure.
constexpr uint32_t RESTART_DELAY_SEC =         10;

// Dump the AVI structure to serial after each recording. Debug only: it costs a
// full extra read of the file.
constexpr bool ENABLE_AVI_ANALYSIS =        false;

// The brownout detector resets the chip when the 3.3V rail sags, which is what
// stops it writing corrupt data to the SD card. The original code disabled it
// unconditionally, which hid genuine supply problems.
//
// Leave this false. If the board now resets under load, that is real
// information: the supply cannot hold up during WiFi TX bursts (~300 mA peak)
// and wants more bulk capacitance at the module, not a masked detector.
constexpr bool DISABLE_BROWNOUT_DETECTOR =  false;

// ---------------------------------------------------------------------------
// Deep sleep
// ---------------------------------------------------------------------------

// Master switch. False keeps the board awake polling the sensor, which is only
// useful while debugging on a bench supply.
constexpr bool ENABLE_DEEP_SLEEP =           true;

// Logic level at PIN_MOTION that means "motion detected".
//
// The sensor does NOT connect to GPIO 13 directly. Its active-high output
// drives an NPN inverter (see README) so the pin idles HIGH and pulls LOW on
// motion. That inversion is not cosmetic -- PIN_MOTION is GPIO 13, which is
// also SD_MMC D3. A card samples D3 while initialising and comes up in SPI
// mode if it is held LOW, so SD_MMC.begin() then fails with a 0x107 timeout.
// An HC-SR501 actively drives its output low while idle, which broke the card
// mount on most boots until the inverter was added.
//
// Set to 1 only for a sensor wired straight to GPIO 13, and expect intermittent
// SD mount failures if you do.
constexpr int MOTION_ACTIVE_LEVEL =             0;  // 0 = LOW (inverted), 1 = HIGH (direct)

// The sensor holds its output high for a while after it triggers. Arming the
// ext0 wake while the pin is still high causes an immediate re-wake, so sleep
// is deferred until it falls. Kept short: this is a busy-wait at full current
// (~150 mA), so 60 s here would cost ~2.5 mAh, over half of a whole recording.
constexpr uint32_t MOTION_CLEAR_TIMEOUT_MS = 10000;

// If the sensor is STILL asserted when that expires, arming ext0 would wake the
// board immediately and spin forever. Sleep on a timer instead and re-check
// later, which costs deep-sleep current rather than active current.
constexpr uint32_t STUCK_SENSOR_RECHECK_SEC = 60;

// ---------------------------------------------------------------------------
// Motion confirmation
//
// Radar false triggers are what actually determine battery life: a full
// record-and-upload costs ~4.3 mAh, while waking, checking one frame and going
// back to sleep costs ~0.1 mAh. Confirming before committing to a recording is
// worth roughly a 40x reduction on every false trigger.
// ---------------------------------------------------------------------------

// DISABLED, deliberately. This was designed for the HLK-LD1020 radar, which was
// expected to false-trigger often enough that filtering justified the
// complexity. On the HC-SR501 it does more harm than good.
//
// Measured on the bench: real waves scored 0 and 1 changed cells out of 192,
// mean difference 1. The cause is latency, not tuning -- PIR trigger to wake is
// ~1 s, camera init another ~0.5 s, so the camera opens its eyes roughly two
// seconds after the motion that woke it. Someone walking past is long gone, and
// the new frame matches the stale baseline exactly. No threshold fixes that;
// anything low enough to catch it would fire on sensor noise.
//
// A PIR only responds to moving body heat, so its false-trigger rate is far
// lower than the radar's -- and still unmeasured. Missing a real intruder is
// much worse than an occasional wasted recording, so record unconditionally
// until step 3 shows the false-trigger rate actually warrants filtering.
constexpr bool ENABLE_MOTION_CONFIRMATION = false;

// A frame is reduced to a coarse luminance grid small enough to survive in RTC
// memory across deep sleep (16 x 12 = 192 bytes).
constexpr uint32_t SIGNATURE_COLS =            16;
constexpr uint32_t SIGNATURE_ROWS =            12;
constexpr uint32_t SIGNATURE_SIZE = SIGNATURE_COLS * SIGNATURE_ROWS;

// Primary test: how many grid cells changed substantially.
//
// Averaging the change across all 192 cells does not work for localised motion.
// A hand fills only 3-4 cells, so measured waves produced mean differences of
// just 2-5 while a person was plainly in frame. Counting cells that moved a
// lot is far more sensitive to a real intruder, and still ignores uniform
// lighting drift because that shifts every cell only slightly.
constexpr uint32_t CELL_DELTA_THRESHOLD =      18;  // per-cell change that counts as "moved"
constexpr uint32_t MIN_CHANGED_CELLS =          3;  // of SIGNATURE_SIZE (192) needed to confirm

// Secondary test: mean absolute difference across the whole grid. Catches
// whole-scene changes a cell count would miss -- lights switching on, a door
// opening onto daylight.
constexpr uint32_t MOTION_THRESHOLD =          12;

// Below this mean luminance the scene is too dark to compare meaningfully, so
// confirmation fails OPEN and records anyway. Without this the camera would go
// blind at night: every frame reads near-black, every diff reads as no motion,
// and nothing is ever recorded.
constexpr uint32_t DARK_SCENE_THRESHOLD =      18;

// The first frames after a cold camera start are badly exposed while auto-gain
// settles; discard this many before sampling.
constexpr uint32_t SIGNATURE_WARMUP_FRAMES =    3;

// ---------------------------------------------------------------------------
// Storage
// ---------------------------------------------------------------------------

// Refuse to start a recording below this much free space, deleting the oldest
// clips until there is room. A 30 s VGA clip runs roughly 8-12 MB.
constexpr uint64_t MIN_FREE_BYTES = 32ULL * 1024 * 1024;

// Upper bound on clips uploaded per wake, so a long backlog cannot hold the
// board awake indefinitely and flatten the pack.
constexpr uint32_t MAX_UPLOADS_PER_WAKE =      10;

// Ceiling on how many recordings are tracked when listing the card.
constexpr uint32_t MAX_TRACKED_RECORDINGS =    64;

// Cards can be slow to answer on a cold start; retry before rebooting.
constexpr int SD_MOUNT_ATTEMPTS =               3;
constexpr uint32_t SD_MOUNT_RETRY_DELAY_MS =  300;

// Recording filename prefix. Names are zero-padded so lexical order matches
// chronological order, which is what makes "delete the oldest" a string
// comparison rather than a timestamp lookup (the RTC is unset before NTP).
constexpr const char* RECORDING_PREFIX = "/rec_";
constexpr const char* RECORDING_SUFFIX = ".avi";

#endif
