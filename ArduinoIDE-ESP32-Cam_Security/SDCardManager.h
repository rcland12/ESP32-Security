#ifndef SDCARD_MANAGER_H
#define SDCARD_MANAGER_H

#include <FS.h>
#include <SD_MMC.h>

class SDCardManager {
public:
  SDCardManager(int csPin = 5);
  bool init(bool useOneLineModeForSD = true);
  bool remount();

  File openFile(const char* path, const char* mode);
  bool deleteFile(const char* path);
  bool exists(const char* path);
  size_t getFreeSpace();

  static bool writeInt(File& file, uint32_t value);
  static bool writeBytes(File& file, const uint8_t* data, size_t length);

  using ProgressCallback = void (*)(size_t bytesProcessed, size_t totalBytes);

private:
  const int _csPin;
  bool _initialized;
};

#endif