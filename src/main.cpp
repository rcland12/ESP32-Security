#include "ConfigManager.h"
#include "NetworkManager.h"
#include "SDCardManager.h"
#include "soc/rtc_cntl_reg.h"
#include "VideoRecorder.h"
#include <Arduino.h>

const int pirPin = 12;
const int SD_CS = 5;
const int frameRate = 10;
const int recordingTimeSeconds = 10;

SDCardManager sdManager(SD_CS);
ConfigManager configManager(sdManager);
VideoRecorder videoRecorder(sdManager, frameRate, recordingTimeSeconds);
NetworkManager* networkManager = nullptr;

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  delay(1000);

  if(!sdManager.init()) {
    Serial.println("SD Card Mount Failed");
    return;
  }

  Config config = configManager.loadConfig();
  if (!config.isValid()) {
    Serial.println("Failed to load valid configuration!");
    return;
  }

  networkManager = new NetworkManager(
    sdManager,
    config.wifi_ssid.c_str(),
    config.wifi_password.c_str(),
    config.server_url.c_str()
  );

  pinMode(pirPin, INPUT_PULLDOWN);
  Serial.println("Calibrating PIR sensor...");
  delay(2000);
  Serial.println("PIR sensor ready!");

  if (!videoRecorder.initCamera()) {
    Serial.println("Camera initialization failed!");
    return;
  }
  Serial.println("Camera initialized successfully");

  if (!networkManager->initWiFi()) {
    Serial.println("WiFi initialization failed!");
    return;
  }
}

void loop() {
  if (digitalRead(pirPin) == HIGH) {
    Serial.println("Motion detected! Recording video...");

    String filename = "/motion_" + String(millis()) + ".avi";

    if (videoRecorder.recordVideo(filename.c_str())) {
      Serial.println("Video recorded successfully");
      videoRecorder.analyzeAviFile(filename.c_str());

      if (networkManager && networkManager->uploadFile(filename.c_str())) {
        Serial.println("Video uploaded successfully");
        // sdManager.deleteFile(filename.c_str());
      } else {
        Serial.println("Failed to upload video");
      }
    } else {
      Serial.println("Failed to record video");
    }

    delay(5000);
  }

  delay(100);
}
