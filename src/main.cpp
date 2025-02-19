#define FTPCLIENT_GENERIC_IMPL_H
#include <FTPClient_Generic_Impl.h>

#include <Arduino.h>
#include "ConfigManager.h"
#include "NetworkManager.h"
#include "SDCardManager.h"
#include "VideoRecorder.h"
#include "soc/rtc_cntl_reg.h"

const int pirPin = 12;
const int SD_CS = 5;
const int recordingTimeSeconds = 10;
const int frameRate = 1;

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
    config.ftp_server.c_str(),
    config.ftp_user.c_str(),
    config.ftp_password.c_str(),
    config.ftp_port
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
  // if (!sdManager.remount()) {
  //   Serial.println("SD Card remount needed");
  //   delay(1000);
  //   return;
  // }

  if (digitalRead(pirPin) == HIGH) {
    Serial.println("Motion detected! Recording video...");

    String filename = "/motion_" + String(millis()) + ".avi";

    if (videoRecorder.recordVideo(filename.c_str())) {
      Serial.println("Video recorded successfully");
      videoRecorder.analyzeAviFile(filename.c_str());

      if (networkManager && networkManager->uploadToFTP(filename.c_str())) {
        Serial.println("Video uploaded successfully");
        // Optionally delete the local file after successful upload
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
