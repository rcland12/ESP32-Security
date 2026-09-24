// Camera diagnostic sweep. Not part of the application.
//
// esp_camera_init() succeeding only proves SCCB (I2C) reached the sensor; it
// says nothing about the parallel data bus. This probe walks the configuration
// space and reports which combinations actually yield a frame, so a capture
// failure can be attributed to clock rate, frame size, buffer strategy, or
// hardware rather than guessed at.
//
//   pio run -e camera_probe -t upload && pio device monitor -e camera_probe

#include "PinDefinitions.h"
#include <Arduino.h>
#include <esp_camera.h>

struct Combo {
  const char* label;
  int xclk;
  pixformat_t format;
  framesize_t size;
  int fbCount;
  camera_grab_mode_t grab;
  camera_fb_location_t location;
};

static const Combo kCombos[] = {
  {"10MHz JPEG  QQVGA fb1 WHEN_EMPTY", 10000000, PIXFORMAT_JPEG, FRAMESIZE_QQVGA, 1, CAMERA_GRAB_WHEN_EMPTY, CAMERA_FB_IN_PSRAM},
  {"10MHz JPEG  QVGA  fb1 WHEN_EMPTY", 10000000, PIXFORMAT_JPEG, FRAMESIZE_QVGA,  1, CAMERA_GRAB_WHEN_EMPTY, CAMERA_FB_IN_PSRAM},
  {"10MHz JPEG  VGA   fb1 WHEN_EMPTY", 10000000, PIXFORMAT_JPEG, FRAMESIZE_VGA,   1, CAMERA_GRAB_WHEN_EMPTY, CAMERA_FB_IN_PSRAM},
  {"10MHz JPEG  VGA   fb2 WHEN_EMPTY", 10000000, PIXFORMAT_JPEG, FRAMESIZE_VGA,   2, CAMERA_GRAB_WHEN_EMPTY, CAMERA_FB_IN_PSRAM},
  {"10MHz JPEG  VGA   fb2 LATEST    ", 10000000, PIXFORMAT_JPEG, FRAMESIZE_VGA,   2, CAMERA_GRAB_LATEST,     CAMERA_FB_IN_PSRAM},
  {"10MHz JPEG  VGA   fb1 DRAM      ", 10000000, PIXFORMAT_JPEG, FRAMESIZE_VGA,   1, CAMERA_GRAB_WHEN_EMPTY, CAMERA_FB_IN_DRAM},
  {"20MHz JPEG  VGA   fb2 LATEST    ", 20000000, PIXFORMAT_JPEG, FRAMESIZE_VGA,   2, CAMERA_GRAB_LATEST,     CAMERA_FB_IN_PSRAM},
  {" 8MHz JPEG  VGA   fb2 WHEN_EMPTY",  8000000, PIXFORMAT_JPEG, FRAMESIZE_VGA,   2, CAMERA_GRAB_WHEN_EMPTY, CAMERA_FB_IN_PSRAM},
  {"10MHz GRAY  QQVGA fb1 WHEN_EMPTY", 10000000, PIXFORMAT_GRAYSCALE, FRAMESIZE_QQVGA, 1, CAMERA_GRAB_WHEN_EMPTY, CAMERA_FB_IN_PSRAM},
};

static void fillPins(camera_config_t& c) {
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer = LEDC_TIMER_0;
  c.pin_d0 = Y2_GPIO_NUM;  c.pin_d1 = Y3_GPIO_NUM;
  c.pin_d2 = Y4_GPIO_NUM;  c.pin_d3 = Y5_GPIO_NUM;
  c.pin_d4 = Y6_GPIO_NUM;  c.pin_d5 = Y7_GPIO_NUM;
  c.pin_d6 = Y8_GPIO_NUM;  c.pin_d7 = Y9_GPIO_NUM;
  c.pin_xclk = XCLK_GPIO_NUM;   c.pin_pclk = PCLK_GPIO_NUM;
  c.pin_vsync = VSYNC_GPIO_NUM; c.pin_href = HREF_GPIO_NUM;
  c.pin_sscb_sda = SIOD_GPIO_NUM; c.pin_sscb_scl = SIOC_GPIO_NUM;
  c.pin_pwdn = PWDN_GPIO_NUM;   c.pin_reset = RESET_GPIO_NUM;
}

static void runCombo(const Combo& combo) {
  camera_config_t config = {};
  fillPins(config);
  config.xclk_freq_hz = combo.xclk;
  config.pixel_format = combo.format;
  config.frame_size = combo.size;
  config.jpeg_quality = 12;
  config.fb_count = combo.fbCount;
  config.grab_mode = combo.grab;
  config.fb_location = combo.location;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("  %s : INIT FAILED 0x%x\n", combo.label, err);
    return;
  }

  sensor_t* s = esp_camera_sensor_get();
  uint16_t pid = s ? s->id.PID : 0;

  int ok = 0;
  size_t firstLen = 0;
  for (int i = 0; i < 4; i++) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb) {
      ok++;
      if (!firstLen) firstLen = fb->len;
      esp_camera_fb_return(fb);
    }
    delay(120);
  }

  Serial.printf("  %s : %d/4 frames, %u bytes, sensor PID 0x%04X\n",
                combo.label, ok, (unsigned)firstLen, pid);

  esp_camera_deinit();
  delay(400);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=== ESP32-CAM capture probe ===");
  Serial.printf("PSRAM: %s (%u bytes)\n",
                psramFound() ? "yes" : "no", (unsigned)ESP.getPsramSize());
  Serial.printf("Free heap: %u\n", (unsigned)ESP.getFreeHeap());
  Serial.println("A sensor PID of 0x0000 means SCCB never identified the sensor.");
  Serial.println("OV2640 reports PID 0x2642.\n");

  for (const Combo& combo : kCombos) {
    runCombo(combo);
  }

  Serial.println("\n=== probe complete ===");
}

void loop() {
  delay(1000);
}
