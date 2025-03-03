#include "NetworkManager.h"

NetworkManager::NetworkManager(
  SDCardManager& sdManager,
  const char* ssid, 
  const char* password,
  const char* server_url
):
  _sdManager(sdManager),
  _ssid(ssid),
  _password(password),
  _server_url(server_url)
{}

NetworkManager::~NetworkManager() {}

bool NetworkManager::initWiFi(int maxAttempts) {
  Serial.println("Initializing WiFi...");
  WiFi.mode(WIFI_STA);

  WiFi.begin(_ssid, _password);
  Serial.printf("Connecting to %s", _ssid);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi Connected!");
    Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
    return true;
  } else {
    Serial.println("WiFi Connection Failed!");
    return false;
  }
}

bool NetworkManager::isWiFiConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

String NetworkManager::getIPAddress() const {
  return WiFi.localIP().toString();
}

bool NetworkManager::uploadFile(const char* filename) {
  if (!isWiFiConnected()) {
    Serial.println("WiFi not connected, cannot upload");
    return false;
  }

  File uploadFile = _sdManager.openFile(filename, FILE_READ);
  if (!uploadFile) {
    Serial.println("Failed to open local file");
    return false;
  }

  size_t fileSize = uploadFile.size();
  Serial.printf("Uploading file: %s (Size: %d bytes)\n", filename, fileSize);

  HTTPClient http;
  String justFilename = String(filename).substring(String(filename).lastIndexOf('/') + 1);

  String uploadUrl = _server_url;
  if (!uploadUrl.endsWith("/")) {
    uploadUrl += "/";
  }
  uploadUrl += justFilename;
  
  Serial.printf("Uploading to URL: %s\n", uploadUrl.c_str());
  
  http.begin(uploadUrl);

  http.addHeader("Content-Type", "video/x-msvideo"); 

  Serial.println("Starting HTTP PUT request...");
  int httpCode = http.sendRequest("PUT", &uploadFile, fileSize);

  bool success = false;
  if (httpCode > 0) {
    Serial.printf("HTTP Response code: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
      Serial.println("Upload successful");
      success = true;
    } else {
      Serial.println("Upload failed with HTTP code: " + String(httpCode));
    }

    String response = http.getString();
    if (response.length() > 0) {
      Serial.println("Server response: ");
      Serial.println(response);
    }
  } else {
    Serial.printf("HTTP request failed: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  uploadFile.close();
  
  return success;
}
