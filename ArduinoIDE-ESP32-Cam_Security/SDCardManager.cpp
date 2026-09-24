#include "ConfigSettings.h"
#include "SDCardManager.h"

bool SDCardManager::init(bool useOneLineModeForSD) {
  if (_initialized) {
    return true;
  }

  // 1-bit mode by default: it leaves GPIO 4, 12 and 13 free, which is what
  // makes GPIO 13 available as the motion-sensor wake pin.
  //
  // Cards are occasionally slow to respond on a cold start, so give it a few
  // attempts before declaring failure and rebooting the board.
  bool mounted = false;
  for (int attempt = 1; attempt <= SD_MOUNT_ATTEMPTS && !mounted; attempt++) {
    mounted = SD_MMC.begin("/sdcard", useOneLineModeForSD);
    if (!mounted) {
      Serial.printf("SD mount attempt %d/%d failed\n", attempt, SD_MOUNT_ATTEMPTS);
      SD_MMC.end();
      delay(SD_MOUNT_RETRY_DELAY_MS);
    }
  }

  if (!mounted) {
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

uint64_t SDCardManager::getFreeSpace() {
  if (!_initialized && !init()) {
    return 0;
  }
  return SD_MMC.totalBytes() - SD_MMC.usedBytes();
}

size_t SDCardManager::listRecordings(String* out, size_t maxCount,
                                     const char* prefix, const char* suffix) {
  if (!_initialized && !init()) {
    return 0;
  }

  File root = SD_MMC.open("/");
  if (!root || !root.isDirectory()) {
    Serial.println("Failed to open card root");
    return 0;
  }

  size_t count = 0;
  for (File entry = root.openNextFile(); entry; entry = root.openNextFile()) {
    if (entry.isDirectory()) {
      entry.close();
      continue;
    }

    String name = entry.name();
    entry.close();

    // openNextFile() reports bare names on some core versions and rooted paths
    // on others; normalise so the prefix comparison behaves either way.
    if (!name.startsWith("/")) {
      name = "/" + name;
    }
    if (!name.startsWith(prefix) || !name.endsWith(suffix)) {
      continue;
    }

    if (count >= maxCount) {
      Serial.printf("More than %u recordings on card; only the oldest %u are tracked\n",
                    (unsigned)maxCount, (unsigned)maxCount);
      break;
    }

    // Insertion sort: the list is small and this keeps oldest-first ordering
    // without a second pass.
    size_t pos = count;
    while (pos > 0 && out[pos - 1] > name) {
      out[pos] = out[pos - 1];
      pos--;
    }
    out[pos] = name;
    count++;
  }

  root.close();
  return count;
}

bool SDCardManager::ensureFreeSpace(uint64_t required, const char* prefix, const char* suffix) {
  if (getFreeSpace() >= required) {
    return true;
  }

  String names[MAX_TRACKED_RECORDINGS];
  size_t count = listRecordings(names, MAX_TRACKED_RECORDINGS, prefix, suffix);

  for (size_t i = 0; i < count && getFreeSpace() < required; i++) {
    Serial.printf("Low on space, deleting oldest recording: %s\n", names[i].c_str());
    deleteFile(names[i].c_str());
  }

  bool ok = getFreeSpace() >= required;
  if (!ok) {
    Serial.println("Could not free enough space on the card");
  }
  return ok;
}

