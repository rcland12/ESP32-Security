#include "NetworkManager.h"

NetworkManager::NetworkManager(
  SDCardManager& sdManager,
  const char* ssid, 
  const char* password,
  const char* server_url,
  const char* camera_id
):
  _sdManager(sdManager),
  _ssid(ssid),
  _password(password),
  _server_url(server_url),
  _camera_id(camera_id)
{}

NetworkManager::~NetworkManager() {}

bool NetworkManager::initWiFi(int maxAttempts, uint8_t channel, const uint8_t* bssid) {
  Serial.println("Initializing WiFi...");
  WiFi.mode(WIFI_STA);

  // Associating with a known channel and BSSID skips scanning every channel,
  // which is several seconds of radio-on time on every single wake.
  if (channel != 0 && bssid != nullptr) {
    Serial.printf("Fast connect: channel %u\n", channel);
    WiFi.begin(_ssid.c_str(), _password.c_str(), channel, bssid);
  } else {
    WiFi.begin(_ssid.c_str(), _password.c_str());
  }
  Serial.printf("Connecting to %s", _ssid.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("WiFi Connected on channel %u\n", WiFi.channel());
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

bool NetworkManager::ensureConnected(int maxAttempts) {
  if (isWiFiConnected()) {
    return true;
  }
  Serial.println("WiFi not connected, reconnecting...");
  return initWiFi(maxAttempts);
}

void NetworkManager::disconnect() {
  // Powering the radio down before sleep matters: an associated radio left on
  // draws far more than the rest of the board does asleep.
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
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

  uploadUrl += _camera_id;
  uploadUrl += "/";
  uploadUrl += justFilename;
  
  Serial.printf("Uploading to URL: %s\n", uploadUrl.c_str());
  
  http.begin(uploadUrl);

  http.addHeader("Content-Type", "video/x-msvideo"); 

  Serial.println("Starting HTTP PUT request...");
  int httpCode = http.sendRequest("PUT", &uploadFile, fileSize);

  bool success = false;
  if (httpCode > 0) {
    Serial.printf("HTTP Response code: %d\n", httpCode);
    // Any 2xx means the server took the file. nginx's DAV module returns 201
    // when it creates a file but 204 when it overwrites one, so checking only
    // for 200/201 made every re-upload look like a failure -- the clip would be
    // kept and retried forever even though the server already had it.
    if (httpCode >= 200 && httpCode < 300) {
      Serial.println("Upload successful");
      success = true;
    } else {
      Serial.println("Upload failed with HTTP code: " + String(httpCode));
    }

    // Only read a body when one actually exists. A 204 carries no body by
    // definition, and getString() on it blocks until the socket times out --
    // that stall was costing ~60 s at full current after every single upload.
    // Errors are worth the read; successes are not.
    if (!success && httpCode != HTTP_CODE_NO_CONTENT && http.getSize() != 0) {
      String response = http.getString();
      if (response.length() > 0) {
        Serial.println("Server response: ");
        Serial.println(response);
      }
    }
  } else {
    Serial.printf("HTTP request failed: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  uploadFile.close();
  
  return success;
}

uint8_t NetworkManager::currentChannel() const {
  return WiFi.channel();
}

const uint8_t* NetworkManager::currentBssid() const {
  return WiFi.BSSID();
}
