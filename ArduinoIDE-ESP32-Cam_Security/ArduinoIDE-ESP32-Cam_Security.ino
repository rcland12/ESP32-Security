#include "soc/rtc_cntl_reg.h"
#include "ConfigSettings.h"
#include "PinDefinitions.h"
#include <ConfigManager.h>
#include <NetworkManager.h>
#include <SDCardManager.h>
#include <VideoRecorder.h>
#include <WiFi.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

// Everything happens in setup(): the board wakes on motion, does one unit of
// work, and deep sleeps again. loop() only runs when sleep is disabled for
// bench debugging.

SDCardManager sdManager;
ConfigManager configManager(sdManager);
VideoRecorder videoRecorder(sdManager, FRAME_RATE, RECORD_TIME_SEC);
NetworkManager* networkManager = nullptr;

// Survives deep sleep in RTC memory. bootCount doubles as the filename counter:
// millis() restarts from zero on every wake, so the old millis()-based names
// would have collided on every single recording once sleep was enabled.
RTC_DATA_ATTR uint32_t bootCount = 0;
RTC_DATA_ATTR uint8_t referenceSignature[SIGNATURE_SIZE];
RTC_DATA_ATTR bool referenceValid = false;

// Cached association details. Re-associating with a known channel and BSSID
// skips scanning every channel, which is seconds of radio-on time per wake.
RTC_DATA_ATTR uint8_t wifiChannel = 0;
RTC_DATA_ATTR uint8_t wifiBssid[6] = {0};

static void enterDeepSleep();

static void fatal(const char* reason) {
  Serial.printf("FATAL: %s - restarting in %u s\n", reason, RESTART_DELAY_SEC);
  Serial.flush();
  delay(RESTART_DELAY_SEC * 1000UL);
  ESP.restart();
}

static inline bool motionActive() {
  return digitalRead(PIN_MOTION) == MOTION_ACTIVE_LEVEL;
}

// The internal pull must hold the pin at its INACTIVE level, so a disconnected
// sensor reads as "no motion" rather than latching on. With the inverter that
// inactive level is HIGH, which conveniently is also what SD_MMC's D3 line
// needs while the card initialises.
static void configureMotionPin() {
  pinMode(PIN_MOTION, MOTION_ACTIVE_LEVEL == 0 ? INPUT_PULLUP : INPUT_PULLDOWN);
}

// The sensor holds its output asserted for a while after triggering. Arming
// ext0 while it is still asserted wakes the board immediately, so defer sleep
// until it releases. Returns true once released, false if it is still asserted
// when the timeout expires.
static bool waitForMotionClear() {
  configureMotionPin();

  unsigned long deadline = millis() + MOTION_CLEAR_TIMEOUT_MS;
  while (motionActive() && millis() < deadline) {
    delay(100);
  }
  return !motionActive();
}

static void enterDeepSleep() {
  if (!ENABLE_DEEP_SLEEP) {
    Serial.println("Deep sleep disabled; idling instead");
    return;
  }

  if (networkManager) networkManager->disconnect();
  videoRecorder.deinitCamera();

  bool motionCleared = waitForMotionClear();

  // Hold the camera powered down through sleep. Without the hold the pin floats
  // once the digital core stops driving it, and the OV2640 keeps drawing.
  pinMode(PWDN_GPIO_NUM, OUTPUT);
  digitalWrite(PWDN_GPIO_NUM, HIGH);
  gpio_hold_en((gpio_num_t)PWDN_GPIO_NUM);
  gpio_deep_sleep_hold_en();

  // Hold the pin at its inactive level through sleep, or ext0 fires instantly.
  if (MOTION_ACTIVE_LEVEL == 0) {
    rtc_gpio_pulldown_dis((gpio_num_t)PIN_MOTION);
    rtc_gpio_pullup_en((gpio_num_t)PIN_MOTION);
  } else {
    rtc_gpio_pullup_dis((gpio_num_t)PIN_MOTION);
    rtc_gpio_pulldown_en((gpio_num_t)PIN_MOTION);
  }

  if (motionCleared) {
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_MOTION, MOTION_ACTIVE_LEVEL);
    Serial.printf("Sleeping on motion wake. Boot count %u\n", bootCount);
  } else {
    // Still asserted: ext0 would fire the instant we sleep. Use a timer so the
    // wait happens at deep-sleep current instead of spinning at full power.
    esp_sleep_enable_timer_wakeup(STUCK_SENSOR_RECHECK_SEC * 1000000ULL);
    Serial.printf("Sensor still asserted; sleeping %u s on timer. Boot count %u\n",
                  STUCK_SENSOR_RECHECK_SEC, bootCount);
  }

  // flush() alone does not reliably drain the UART FIFO before the core stops,
  // which truncates the last log line and makes sleep look like a hang.
  Serial.flush();
  delay(50);
  esp_deep_sleep_start();
}

// Returns true if the scene changed enough to be worth a 30 s recording.
// Deliberately fails OPEN: any inability to judge results in a recording.
static bool evaluateScene() {
  if (!ENABLE_MOTION_CONFIRMATION) return true;

  if (!videoRecorder.initCameraForDetection()) {
    Serial.println("Detection mode failed to start; recording without confirmation");
    return true;
  }

  uint8_t signature[SIGNATURE_SIZE];
  uint32_t meanLuminance = 0;
  if (!videoRecorder.captureSignature(signature, meanLuminance)) {
    Serial.println("Could not sample scene; recording without confirmation");
    return true;
  }

  // Too dark to compare: at night every frame reads near-black and every diff
  // reads as "nothing moved", which would blind the camera exactly when it
  // matters most.
  if (meanLuminance < DARK_SCENE_THRESHOLD) {
    Serial.printf("Scene too dark to confirm (luma %u); recording\n", meanLuminance);
    memcpy(referenceSignature, signature, SIGNATURE_SIZE);
    referenceValid = true;
    return true;
  }

  if (!referenceValid) {
    Serial.println("No reference frame yet; recording and storing baseline");
    memcpy(referenceSignature, signature, SIGNATURE_SIZE);
    referenceValid = true;
    return true;
  }

  uint32_t meanDiff = VideoRecorder::signatureDifference(signature, referenceSignature);
  uint32_t changed = VideoRecorder::changedCellCount(signature, referenceSignature,
                                                     CELL_DELTA_THRESHOLD);

  // Update the baseline either way, so gradual lighting changes do not leave
  // the camera permanently triggering against a stale reference.
  memcpy(referenceSignature, signature, SIGNATURE_SIZE);

  // Both numbers are logged so the thresholds can be tuned from real events
  // rather than guessed at.
  Serial.printf("Scene: %u/%u cells moved >%u (need %u), mean diff %u (need %u), luma %u\n",
                changed, (unsigned)SIGNATURE_SIZE, CELL_DELTA_THRESHOLD, MIN_CHANGED_CELLS,
                meanDiff, MOTION_THRESHOLD, meanLuminance);

  return changed >= MIN_CHANGED_CELLS || meanDiff >= MOTION_THRESHOLD;
}

static bool motionConfirmed() {
  bool confirmed = evaluateScene();

  // The camera MUST be released before the SD card is touched. Leaving it
  // streaming while SD_MMC.begin() runs makes the mount time out with 0x107 --
  // which only ever showed up on motion wakes, because the power-on path mounts
  // the card before the camera is ever started.
  videoRecorder.deinitCamera();

  return confirmed;
}

static String recordingName(uint32_t index) {
  char name[48];
  snprintf(name, sizeof(name), "%s%06u_%02u%s",
           RECORDING_PREFIX, bootCount, index, RECORDING_SUFFIX);
  return String(name);
}

// Uploads everything still on the card, oldest first, deleting only what the
// server actually accepted. Anything that fails stays put and is retried on a
// later wake rather than being lost.
static void uploadPending() {
  if (!networkManager) return;

  String names[MAX_TRACKED_RECORDINGS];
  size_t count = sdManager.listRecordings(names, MAX_TRACKED_RECORDINGS,
                                          RECORDING_PREFIX, RECORDING_SUFFIX);
  if (count == 0) return;

  if (!networkManager->ensureConnected()) {
    Serial.printf("WiFi unavailable; %u recording(s) left queued on the card\n",
                  (unsigned)count);
    return;
  }

  uint32_t uploaded = 0;
  for (size_t i = 0; i < count && uploaded < MAX_UPLOADS_PER_WAKE; i++) {
    if (networkManager->uploadFile(names[i].c_str())) {
      sdManager.deleteFile(names[i].c_str());
      uploaded++;
    } else {
      Serial.printf("Upload failed for %s; keeping it queued\n", names[i].c_str());
      break;  // server or link is down, so stop burning power on the rest
    }
  }

  if (count > uploaded) {
    Serial.printf("%u recording(s) still queued\n", (unsigned)(count - uploaded));
  }
}

void setup() {
  if (DISABLE_BROWNOUT_DETECTOR) {
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  }

  Serial.begin(115200);
  delay(SERIAL_SETTLE_MS);

  bootCount++;
  esp_sleep_wakeup_cause_t wakeCause = esp_sleep_get_wakeup_cause();

  // A timer wake only ever happens because the sensor was still asserted when
  // we went to sleep, so it is a motion candidate too -- let confirmation
  // decide rather than recording unconditionally.
  bool wokeOnMotion = (wakeCause == ESP_SLEEP_WAKEUP_EXT0 ||
                       wakeCause == ESP_SLEEP_WAKEUP_TIMER);
  const char* wakeLabel = wakeCause == ESP_SLEEP_WAKEUP_EXT0   ? "motion wake"
                        : wakeCause == ESP_SLEEP_WAKEUP_TIMER  ? "timer re-check"
                                                               : "power-on / reset";
  Serial.printf("\nBoot %u (%s)\n", bootCount, wakeLabel);

  // Printed on every boot: with several units deployed this is how you tell
  // them apart in the router's lease table when setting up DHCP reservations.
  Serial.printf("WiFi MAC: %s\n", WiFi.macAddress().c_str());

  // Release the camera power-down hold applied before sleeping.
  gpio_deep_sleep_hold_dis();
  gpio_hold_dis((gpio_num_t)PWDN_GPIO_NUM);

  // Release the ext0 wake pulldown before going anywhere near the SD card.
  // PIN_MOTION is SD_MMC D3, and a card that sees D3 low while initialising
  // comes up in SPI mode, making SD_MMC.begin() fail with a 0x107 timeout. The
  // pulldown is applied before sleeping and persists across the wake, so it has
  // to be cleared here on every boot.
  rtc_gpio_deinit((gpio_num_t)PIN_MOTION);
  configureMotionPin();

  // The cheap path. A false trigger costs only this much before going back to
  // sleep, which is what keeps the radar's noisiness affordable.
  if (wokeOnMotion && !motionConfirmed()) {
    Serial.println("No real change in scene; back to sleep");
    enterDeepSleep();
    return;
  }

  // PIN_MOTION is SD_MMC D3. While the sensor is asserted the inverter holds it
  // LOW, and a card that samples D3 low during initialisation comes up in SPI
  // mode -- SD_MMC.begin() then fails with 0x107. So the mount has to wait for
  // the sensor to release.
  //
  // This is not an edge case: on a motion wake the sensor is asserted by
  // definition, and after power-up a PIR spends its whole warm-up period
  // triggering. Both paths would otherwise fail every time. The sensor only
  // holds for about a second, and the card is not needed until recording.
  if (!waitForMotionClear()) {
    Serial.println("Sensor still asserted after timeout; mounting the card anyway");
  }

  if (!sdManager.init()) {
    fatal("SD card mount failed - check the card is seated and FAT32 formatted");
  }

  configureMotionPin();

  Config config = configManager.loadConfig();
  if (!config.isValid()) {
    fatal("config.txt missing or incomplete");
  }

  networkManager = new NetworkManager(
    sdManager,
    config.wifi_ssid.c_str(),
    config.wifi_password.c_str(),
    config.server_url.c_str(),
    config.camera_id.c_str()
  );

  if (!sdManager.ensureFreeSpace(MIN_FREE_BYTES, RECORDING_PREFIX, RECORDING_SUFFIX)) {
    Serial.println("Recording anyway; the card may fill mid-clip");
  }

  if (!videoRecorder.initCamera()) {
    fatal("camera initialization failed");
  }

  String filename = recordingName(0);
  Serial.printf("Recording to %s\n", filename.c_str());

  if (videoRecorder.recordVideo(filename.c_str())) {
    if (ENABLE_AVI_ANALYSIS) {
      videoRecorder.analyzeAviFile(filename.c_str());
    }
  } else {
    Serial.println("Failed to record video");
  }

  // Camera off before the radio comes up: running both at once is the worst
  // case for the supply, and the clip is already safely on the card.
  videoRecorder.deinitCamera();

  // Try the cached channel/BSSID first, then fall back to a full scan if the
  // AP moved channel or the cache is empty.
  bool connected = networkManager->initWiFi(WIFI_FAST_ATTEMPTS, wifiChannel,
                                            wifiChannel ? wifiBssid : nullptr);
  if (!connected && wifiChannel != 0) {
    Serial.println("Fast connect failed; falling back to a full scan");
    wifiChannel = 0;
    connected = networkManager->initWiFi();
  }

  if (!connected) {
    Serial.println("WiFi unavailable; recordings stay queued for a later wake");
  } else {
    wifiChannel = networkManager->currentChannel();
    const uint8_t* bssid = networkManager->currentBssid();
    if (bssid) memcpy(wifiBssid, bssid, sizeof(wifiBssid));
    uploadPending();
  }

  enterDeepSleep();
}

void loop() {
  // Only reached when ENABLE_DEEP_SLEEP is false, for bench debugging.
  if (motionActive()) {
    static uint32_t index = 1;
    String filename = recordingName(index++);

    if (videoRecorder.isCameraReady() || videoRecorder.initCamera()) {
      if (videoRecorder.recordVideo(filename.c_str())) {
        uploadPending();
      }
    }
    delay(5000);
  }
  delay(100);
}
