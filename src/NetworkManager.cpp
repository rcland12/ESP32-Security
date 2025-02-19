#include "NetworkManager.h"

NetworkManager::NetworkManager(
  SDCardManager& sdManager,
  const char* ssid, 
  const char* password,
  const char* ftp_server, 
  const char* ftp_user,
  const char* ftp_pass,
  uint16_t ftp_port
  ):
    _sdManager(sdManager),
    _ssid(ssid),
    _password(password),
    _ftp_server(ftp_server),
    _ftp_user(ftp_user),
    _ftp_pass(ftp_pass),
    _ftp_port(ftp_port)
{}

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

bool NetworkManager::uploadToFTP(const char* filename) {
  static const size_t LOCAL_BUFFER_SIZE = 1024;

  if (!isWiFiConnected()) {
    Serial.println("WiFi not connected, cannot upload");
    return false;
  }

  FTPClient_Generic ftp((char*)_ftp_server, (char*)_ftp_user, (char*)_ftp_pass, 5000);

  Serial.println("Opening FTP connection...");
  ftp.OpenConnection();

  if (!ftp.isConnected()) {
    Serial.println("FTP connection failed");
    return false;
  }

  Serial.println("FTP Connected");

  File uploadFile = _sdManager.openFile(filename, FILE_READ);
  if (!uploadFile) {
    Serial.println("Failed to open local file");
    ftp.CloseConnection();
    return false;
  }

  String remotePath = "/" + String(filename).substring(String(filename).lastIndexOf('/') + 1);
  Serial.printf("Uploading to: %s\n", remotePath.c_str());

  ftp.InitFile(COMMAND_XFER_TYPE_BINARY);
  ftp.NewFile((char*)remotePath.c_str());

  uint8_t buffer[LOCAL_BUFFER_SIZE];
  size_t bytesRead;

  while ((bytesRead = uploadFile.read(buffer, LOCAL_BUFFER_SIZE - 1)) > 0) {
    buffer[bytesRead] = '\0';
    ftp.Write((char*)buffer);
    yield();
  }

  uploadFile.close();
  ftp.CloseFile();
  ftp.CloseConnection();

  Serial.println("Upload completed");
  return true;
}

bool NetworkManager::isWiFiConnected() const {
  return WiFi.status() == WL_CONNECTED;
}

String NetworkManager::getIPAddress() const {
  return WiFi.localIP().toString();
}
