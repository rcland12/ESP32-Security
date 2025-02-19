#include "SDCardManager.h"

SDCardManager::SDCardManager(int csPin) 
  : _csPin(csPin)
  , _initialized(false) {
}

bool SDCardManager::init(bool useOneLineModeForSD) {
  if (_initialized) {
    return true;
  }

  pinMode(_csPin, OUTPUT);
  digitalWrite(_csPin, HIGH);

  if (!SD_MMC.begin("/sdcard", useOneLineModeForSD)) {
    Serial.println("SD Card Mount Failed");
    return false;
  }

  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return false;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  _initialized = true;
  return true;
}

bool SDCardManager::remount() {
  _initialized = false;
  return init();
}

File SDCardManager::openFile(const char* path, const char* mode) {
  if (!_initialized && !init()) {
    return File();
  }
  return SD_MMC.open(path, mode);
}

bool SDCardManager::deleteFile(const char* path) {
  if (!_initialized && !init()) {
    return false;
  }
  return SD_MMC.remove(path);
}

bool SDCardManager::exists(const char* path) {
  if (!_initialized && !init()) {
    return false;
  }
  return SD_MMC.exists(path);
}

size_t SDCardManager::getFreeSpace() {
  if (!_initialized && !init()) {
    return 0;
  }
  return SD_MMC.totalBytes() - SD_MMC.usedBytes();
}

bool SDCardManager::writeInt(File& file, uint32_t value) {
  if (!file) {
    return false;
  }
  return file.write((uint8_t*)&value, 4) == 4;
}

bool SDCardManager::writeBytes(File& file, const uint8_t* data, size_t length) {
  if (!file) {
    return false;
  }
  return file.write(data, length) == length;
}