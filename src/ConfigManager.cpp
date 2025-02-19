#include "ConfigManager.h"

ConfigManager::ConfigManager(SDCardManager& sdManager) 
  : _sdManager(sdManager) {
}

Config ConfigManager::loadConfig(const char* configPath) {
  Config config;

  config.wifi_ssid = "";
  config.wifi_password = "";
  config.ftp_server = "";
  config.ftp_user = "";
  config.ftp_password = "";
  config.ftp_port = 21;

  File configFile = _sdManager.openFile(configPath, FILE_READ);
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
    } else if (key == "FTP_SERVER") {
      config.ftp_server = value;
    } else if (key == "FTP_USER") {
      config.ftp_user = value;
    } else if (key == "FTP_PASSWORD") {
      config.ftp_password = value;
    } else if (key == "FTP_PORT") {
      config.ftp_port = value.toInt();
    }
  }

  configFile.close();
  return config;
}

bool ConfigManager::saveConfig(const Config& config, const char* configPath) {
  File configFile = _sdManager.openFile(configPath, FILE_WRITE);
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  configFile.println("# Security Camera Configuration");
  configFile.println();

  configFile.printf("WIFI_SSID=%s\n", config.wifi_ssid.c_str());
  configFile.printf("WIFI_PASSWORD=%s\n", config.wifi_password.c_str());
  configFile.printf("FTP_SERVER=%s\n", config.ftp_server.c_str());
  configFile.printf("FTP_USER=%s\n", config.ftp_user.c_str());
  configFile.printf("FTP_PASSWORD=%s\n", config.ftp_password.c_str());
  configFile.printf("FTP_PORT=%d\n", config.ftp_port);

  configFile.close();
  return true;
}
