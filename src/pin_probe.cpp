// GPIO diagnostic. Not part of the application.
//
// Watches every header pin that could plausibly carry the motion signal and
// reports which one changes. This answers two questions that are otherwise easy
// to confuse:
//
//   1. "Is the sensor reaching the ESP32 at all?"
//   2. "Which physical pin am I actually touching?"
//
// Touch a GND jumper to the pin you believe is GPIO 13 and the probe will name
// the pin it really is. Adjacent header pins are very easy to miscount.
//
// Does not sleep and never touches the camera or SD card, so a result here
// isolates the signal path from the rest of the firmware.
//
//   pio run -e pin_probe -t upload

#include "ConfigSettings.h"
#include "PinDefinitions.h"
#include <Arduino.h>

// Every free RTC-capable header pin, plus the SD lines, since a miscount most
// likely lands on a neighbour. GPIO 0 is left out (boot strap), as are GPIO 1
// and 3 (the serial link this report travels over) and GPIO 16 (PSRAM).
static const int kPins[] = { 2, 4, 12, 13, 14, 15 };
static constexpr size_t kPinCount = sizeof(kPins) / sizeof(kPins[0]);

static int lastLevel[kPinCount];
static unsigned long lastChange[kPinCount];
static uint32_t transitions[kPinCount];

static const char* levelName(int level) { return level ? "HIGH" : "LOW "; }

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n=== GPIO probe: which pin is moving? ===");
  Serial.printf("Motion pin per firmware : GPIO %d\n", PIN_MOTION);
  Serial.printf("Active level            : %s (MOTION_ACTIVE_LEVEL = %d)\n",
                levelName(MOTION_ACTIVE_LEVEL), MOTION_ACTIVE_LEVEL);
  Serial.println("All pins below are INPUT_PULLUP, so each idles HIGH.\n");
  Serial.println("TEST: touch a GND jumper to the pin you think is GPIO 13.");
  Serial.println("      The probe will name the pin you actually touched.\n");

  Serial.print("Watching: ");
  for (size_t i = 0; i < kPinCount; i++) {
    pinMode(kPins[i], INPUT_PULLUP);
    lastLevel[i] = -1;
    lastChange[i] = 0;
    transitions[i] = 0;
    Serial.printf("GPIO%d%s", kPins[i], i + 1 < kPinCount ? ", " : "\n");
  }
  Serial.println("(GPIO4 also drives the flash LED, so it may light when pulled low)\n");
}

void loop() {
  static unsigned long lastHeartbeat = 0;
  unsigned long now = millis();

  for (size_t i = 0; i < kPinCount; i++) {
    int level = digitalRead(kPins[i]);
    if (level == lastLevel[i]) continue;

    if (lastLevel[i] != -1) {
      transitions[i]++;
      Serial.printf("[%8lu ms] GPIO%-2d -> %s  (was %s for %lu ms)  transitions=%u%s\n",
                    now, kPins[i], levelName(level), levelName(lastLevel[i]),
                    now - lastChange[i], transitions[i],
                    kPins[i] == PIN_MOTION ? "   <-- the motion pin" : "");
    }
    lastLevel[i] = level;
    lastChange[i] = now;
  }

  if (now - lastHeartbeat >= 5000) {
    Serial.printf("[%8lu ms] idle:", now);
    for (size_t i = 0; i < kPinCount; i++) {
      Serial.printf(" GPIO%d=%s", kPins[i], lastLevel[i] ? "H" : "L");
    }
    Serial.println();
    lastHeartbeat = now;
  }

  delay(10);
}
