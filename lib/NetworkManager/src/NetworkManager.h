#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <SDCardManager.h>
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

  // channel/bssid let a wake skip the full-band scan. Pass 0/nullptr for a
  // normal scan-and-associate.
  bool initWiFi(int maxAttempts = 20, uint8_t channel = 0, const uint8_t* bssid = nullptr);

  // Valid only while connected; cache these to speed up the next wake.
  uint8_t currentChannel() const;
  const uint8_t* currentBssid() const;
  bool uploadFile(const char* filename);
  bool isWiFiConnected() const;

  // Reconnects if the association dropped. Without this a single AP reboot
  // stopped the unit uploading until it was power-cycled by hand.
  bool ensureConnected(int maxAttempts = 20);

  void disconnect();

private:
  SDCardManager& _sdManager;
  String _ssid;
  String _password;
  String _server_url;
  String _camera_id;
};

#endif