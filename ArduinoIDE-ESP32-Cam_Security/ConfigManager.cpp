#include "ConfigManager.h"

ConfigManager::ConfigManager(SDCardManager& sdManager) 
  : _sdManager(sdManager) {
}

Config ConfigManager::loadConfig() {
  Config config;

  config.wifi_ssid = "";
  config.wifi_password = "";
  config.server_url = "";

  File configFile = _sdManager.openFile(configFilePath, FILE_READ);
  if (!configFile) {
    Serial.println("Failed to open config file");
    return config;
  }

  while (configFile.available()) {
    String line = configFile.readStringUntil('\n');
    line.trim();

    if (line.length() == 0 || line.startsWith("#")) {
      continue;
    }

    int separatorPos = line.indexOf('=');
    if (separatorPos == -1) {
      continue;
    }

    String key = line.substring(0, separatorPos);
    String value = line.substring(separatorPos + 1);
    key.trim();
    value.trim();

    if (key == "WIFI_SSID") {
      config.wifi_ssid = value;
    } else if (key == "WIFI_PASSWORD") {
      config.wifi_password = value;
    } else if (key == "SERVER_URL") {
      config.server_url = value;
    } else if (key == "CAMERA_ID") {
      config.camera_id = value;
    }
  }

  configFile.close();

  Serial.println("Config loaded:");
  Serial.println("WiFi SSID: " + config.wifi_ssid);
  Serial.println("Server URL: " + config.server_url);
  Serial.println("Camera ID: " + config.camera_id);
  
  return config;
}
