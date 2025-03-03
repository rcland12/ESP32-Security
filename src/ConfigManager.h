#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "SDCardManager.h"
#include <Arduino.h>

struct Config {
  String wifi_ssid;
  String wifi_password;
  String server_url;

  bool isValid() const {
    return wifi_ssid.length() > 0 && 
           wifi_password.length() > 0 && 
           server_url.length() > 0;
  }
};

class ConfigManager {
  public:
    ConfigManager(SDCardManager& sdManager);
    Config loadConfig();
    
  private:
    SDCardManager& _sdManager;
    const char* configFilePath = "/config.txt";
};

#endif