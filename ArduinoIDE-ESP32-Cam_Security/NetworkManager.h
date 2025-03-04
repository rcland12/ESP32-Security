#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "SDCardManager.h"
#include <HTTPClient.h>
#include <WiFi.h>

class NetworkManager {
public:
  NetworkManager(
    SDCardManager& sdManager,
    const char* ssid, 
    const char* password,
    const char* server_url,
    const char* camera_id
  );

  ~NetworkManager();

  bool initWiFi(int maxAttempts = 20);
  bool uploadFile(const char* filename);
  bool isWiFiConnected() const;
  String getIPAddress() const;

private:
  SDCardManager& _sdManager;
  String _ssid;
  String _password;
  String _server_url;
  String _camera_id;

  static const int LOCAL_BUFFER_SIZE = 1024;
};

#endif