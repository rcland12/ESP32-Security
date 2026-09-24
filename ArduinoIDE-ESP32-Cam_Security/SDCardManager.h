#ifndef SDCARD_MANAGER_H
#define SDCARD_MANAGER_H

#include <FS.h>
#include <SD_MMC.h>

// SD_MMC has no chip-select line, so this class takes no CS pin. The previous
// constructor defaulted to GPIO 5, which is the camera's Y2 data pin, and
// driving it as an output collided with the camera bus.
class SDCardManager {
public:
  SDCardManager() = default;
  bool init(bool useOneLineModeForSD = true);

  File openFile(const char* path, const char* mode);
  bool deleteFile(const char* path);
  bool exists(const char* path);
  uint64_t getFreeSpace();

  // Collects recordings from the card root, sorted ascending. Filenames are
  // zero-padded so lexical order is chronological order, which is what makes
  // "oldest" resolvable without a real-time clock.
  size_t listRecordings(String* out, size_t maxCount,
                        const char* prefix, const char* suffix);

  // Frees space by deleting oldest-first until `required` bytes are available.
  // Returns false if it runs out of things to delete.
  bool ensureFreeSpace(uint64_t required, const char* prefix, const char* suffix);

private:
  bool _initialized = false;
};

#endif