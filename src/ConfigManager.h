#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include "SDCardManager.h"

struct Config {
  String wifi_ssid;
  String wifi_password;
  String ftp_server;
  String ftp_user;
  String ftp_password;
  uint16_t ftp_port;

  bool isValid() const {
    return wifi_ssid.length() > 0;
  }
};

class ConfigManager {
public:
  ConfigManager(SDCardManager& sdManager);
  Config loadConfig(const char* configPath = "/config.txt");
  bool saveConfig(const Config& config, const char* configPath = "/config.txt");
    
private:
  SDCardManager& _sdManager;
};

#endif