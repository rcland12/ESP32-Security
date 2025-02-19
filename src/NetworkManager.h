#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>
#include <FTPClient_Generic.h>
#include <WiFi.h>
#include "SDCardManager.h"

class NetworkManager {
public:
  NetworkManager(
    SDCardManager& sdManager,
    const char* ssid, 
    const char* password,
    const char* ftp_server, 
    const char* ftp_user,
    const char* ftp_pass,
    uint16_t ftp_port = 21
  );

  bool initWiFi(int maxAttempts = 20);
  bool uploadToFTP(const char* filename);
  bool isWiFiConnected() const;
  String getIPAddress() const;

private:
  SDCardManager& _sdManager;
  const char* _ssid;
  const char* _password;
  const char* _ftp_server;
  const char* _ftp_user;
  const char* _ftp_pass;
  uint16_t _ftp_port;

  static const int LOCAL_BUFFER_SIZE = 1024;
};

#endif
