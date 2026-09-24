#include "ConfigSettings.h"
#include "VideoRecorder.h"

VideoRecorder::VideoRecorder(SDCardManager& sdManager, int frameRate, int recordingTimeSeconds)
  : sdManager(sdManager), frameRate(frameRate), recordingTimeSeconds(recordingTimeSeconds) {
}

bool VideoRecorder::initCamera() {
  if (!startCamera(PIXFORMAT_JPEG, FRAME_SIZE, 2)) return false;

  sensor_t *s = esp_camera_sensor_get();
  s->set_quality(s, JPEG_QUALITY);
  s->set_framesize(s, FRAME_SIZE);
  s->set_brightness(s, 0);
  s->set_contrast(s, 0);
  s->set_saturation(s, 0);
  s->set_whitebal(s, 1);
  s->set_exposure_ctrl(s, 1);

  Serial.println("Camera ready (JPEG capture mode)");
  return true;
}

bool VideoRecorder::initCameraForDetection() {
  // QQVGA grayscale gives raw 8-bit pixels with no JPEG decode step, and starts
  // far faster than full capture mode. Every false trigger pays this cost, so
  // it is the one path worth keeping cheap.
  if (!startCamera(PIXFORMAT_GRAYSCALE, FRAMESIZE_QQVGA, 1)) return false;

  Serial.println("Camera ready (grayscale detection mode)");
  return true;
}

void VideoRecorder::deinitCamera() {
  if (!cameraReady) return;
  esp_camera_deinit();
  cameraReady = false;
}

bool VideoRecorder::startCamera(pixformat_t format, framesize_t size, int fbCount) {
  deinitCamera();

  // Zero-initialised: fields left unset here (notably grab_mode and
  // fb_location) would otherwise hold stack garbage.
  camera_config_t config = {};

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = XCLK_FREQ_HZ;
  config.pixel_format = format;
  config.frame_size = size;
  config.jpeg_quality = JPEG_QUALITY;

  // Framebuffers live in PSRAM when it exists. Some clone boards ship without
  // it; forcing CAMERA_FB_IN_PSRAM there lets init succeed but leaves every
  // capture returning NULL, so fall back rather than fail silently.
  if (psramFound()) {
    config.fb_count = fbCount;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    Serial.println("WARNING: no PSRAM detected; using DRAM framebuffers");
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  }

  Serial.printf("Camera init: xclk=%d Hz, psram=%s (%u bytes), fb_count=%d\n",
                XCLK_FREQ_HZ, psramFound() ? "yes" : "no",
                (unsigned)ESP.getPsramSize(), config.fb_count);

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }

  cameraReady = true;

  // Prove the sensor can actually produce a frame. Without this, a camera that
  // initialises but never captures reports success and the failure only shows
  // up as an empty recording 30 seconds later.
  camera_fb_t *probe = esp_camera_fb_get();
  if (!probe) {
    Serial.println("Camera initialised but first capture failed");
    Serial.println("  -> suspect XCLK_FREQ_HZ, supply sag, or the ribbon seating");
    deinitCamera();
    return false;
  }
  Serial.printf("Camera self-test OK: %u bytes, %ux%u\n",
                (unsigned)probe->len, (unsigned)probe->width, (unsigned)probe->height);
  esp_camera_fb_return(probe);

  return true;
}

bool VideoRecorder::captureSignature(uint8_t* signature, uint32_t& meanLuminance) {
  meanLuminance = 0;
  if (!cameraReady) return false;

  // Auto-exposure needs a few frames to settle after a cold start; sampling
  // before it does produces a signature that differs from the reference purely
  // because of gain, not because anything moved.
  for (uint32_t i = 0; i < SIGNATURE_WARMUP_FRAMES; i++) {
    camera_fb_t* warm = esp_camera_fb_get();
    if (warm) esp_camera_fb_return(warm);
    delay(30);
  }

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Signature capture failed");
    return false;
  }

  if (fb->format != PIXFORMAT_GRAYSCALE ||
      fb->width < SIGNATURE_COLS || fb->height < SIGNATURE_ROWS) {
    Serial.println("Unexpected frame format for signature");
    esp_camera_fb_return(fb);
    return false;
  }

  const uint32_t cellW = fb->width / SIGNATURE_COLS;
  const uint32_t cellH = fb->height / SIGNATURE_ROWS;
  uint32_t total = 0;

  for (uint32_t row = 0; row < SIGNATURE_ROWS; row++) {
    for (uint32_t col = 0; col < SIGNATURE_COLS; col++) {
      uint32_t sum = 0;
      for (uint32_t y = 0; y < cellH; y++) {
        const uint8_t* src = fb->buf + (row * cellH + y) * fb->width + col * cellW;
        for (uint32_t x = 0; x < cellW; x++) {
          sum += src[x];
        }
      }
      uint8_t value = static_cast<uint8_t>(sum / (cellW * cellH));
      signature[row * SIGNATURE_COLS + col] = value;
      total += value;
    }
  }

  esp_camera_fb_return(fb);
  meanLuminance = total / SIGNATURE_SIZE;
  return true;
}

uint32_t VideoRecorder::signatureDifference(const uint8_t* a, const uint8_t* b) {
  uint32_t sum = 0;
  for (uint32_t i = 0; i < SIGNATURE_SIZE; i++) {
    sum += (a[i] > b[i]) ? (a[i] - b[i]) : (b[i] - a[i]);
  }
  return sum / SIGNATURE_SIZE;
}

uint32_t VideoRecorder::changedCellCount(const uint8_t* a, const uint8_t* b, uint32_t cellDelta) {
  uint32_t changed = 0;
  for (uint32_t i = 0; i < SIGNATURE_SIZE; i++) {
    uint32_t delta = (a[i] > b[i]) ? (a[i] - b[i]) : (b[i] - a[i]);
    if (delta > cellDelta) changed++;
  }
  return changed;
}

void VideoRecorder::framePixels(framesize_t size, uint32_t &width, uint32_t &height) {
  switch (size) {
    case FRAMESIZE_QVGA: width = 320;  height = 240; break;
    case FRAMESIZE_VGA:  width = 640;  height = 480; break;
    case FRAMESIZE_SVGA: width = 800;  height = 600; break;
    case FRAMESIZE_XGA:  width = 1024; height = 768; break;
    case FRAMESIZE_HD:   width = 1280; height = 720; break;
    default:             width = 640;  height = 480; break;
  }
}

camera_fb_t* VideoRecorder::captureStableFrame() {
  camera_fb_t *fb = NULL;
  int retry = 0;

  while (retry < 5 && fb == NULL) {
    fb = esp_camera_fb_get();

    if (fb == NULL) {
      Serial.println("Camera capture failed, retrying...");
      retry++;
      delay(50);
      continue;
    }

    // Reject anything that is not a JPEG SOI, so a glitched frame never
    // reaches the muxer.
    if (fb->len < 100 || fb->buf[0] != 0xFF || fb->buf[1] != 0xD8) {
      Serial.println("Invalid JPEG data");
      esp_camera_fb_return(fb);
      fb = NULL;
      retry++;
      delay(50);
      continue;
    }

    return fb;
  }

  return fb;
}

void VideoRecorder::writeAviHeader(File &file, uint32_t width, uint32_t height,
                                   uint32_t fps, uint32_t frameCount) {
  uint8_t header[Avi::kHeaderSize];
  Avi::buildHeader(header, width, height, fps, frameCount);
  file.write(header, sizeof(header));
  file.flush();
}

void VideoRecorder::patchU32(File &file, uint32_t offset, uint32_t value) {
  uint8_t buf[4];
  Avi::putU32(buf, value);
  file.seek(offset);
  file.write(buf, sizeof(buf));
}

void VideoRecorder::updateAviHeader(File &file, uint32_t totalFrames, uint32_t moviPayload) {
  uint32_t fileSize = file.size();

  Serial.printf("Updating headers: frames=%u, file_size=%u, movi_payload=%u\n",
                totalFrames, fileSize, moviPayload);

  // moviPayload already includes the "movi" FOURCC, so bytes-per-second is
  // derived from it directly rather than from a peak-frame estimate.
  uint32_t bytesPerSec = totalFrames ? (moviPayload * frameRate) / totalFrames : 0;

  patchU32(file, Avi::kRiffSizeOffset, fileSize - 8);
  patchU32(file, Avi::kMicroSecPerFrame, frameRate ? 1000000u / frameRate : 0);
  patchU32(file, Avi::kMaxBytesPerSec, bytesPerSec);
  patchU32(file, Avi::kAvihTotalFrames, totalFrames);
  patchU32(file, Avi::kStrhScale, 1);
  patchU32(file, Avi::kStrhRate, frameRate);
  patchU32(file, Avi::kStrhLength, totalFrames);
  patchU32(file, Avi::kMoviSizeOffset, moviPayload);

  file.flush();
}

bool VideoRecorder::writeFrame(File &file, camera_fb_t *fb) {
  uint8_t prefix[Avi::kChunkPrefix];
  Avi::buildFrameChunkPrefix(prefix, fb->len);

  if (file.write(prefix, sizeof(prefix)) != sizeof(prefix)) return false;
  if (file.write(fb->buf, fb->len) != fb->len) return false;

  // Chunks are word-aligned. The pad byte shifts subsequent offsets but is
  // never counted in the chunk's reported length.
  if (fb->len & 1) {
    const uint8_t pad = 0;
    if (file.write(&pad, 1) != 1) return false;
  }

  return true;
}

void VideoRecorder::writeIndex(File &file, uint32_t frameCount, const uint32_t *frameLengths) {
  uint8_t prefix[Avi::kChunkPrefix];
  Avi::putFourCC(prefix, "idx1");
  Avi::putU32(prefix + 4, frameCount * Avi::kIndexEntrySize);
  file.write(prefix, sizeof(prefix));

  Serial.printf("Writing index with %u entries\n", frameCount);

  uint32_t offset = Avi::kFirstFrameIndexOffset;
  for (uint32_t i = 0; i < frameCount; i++) {
    uint8_t entry[Avi::kIndexEntrySize];
    Avi::buildIndexEntry(entry, offset, frameLengths[i]);
    file.write(entry, sizeof(entry));
    offset += Avi::kChunkPrefix + Avi::paddedLength(frameLengths[i]);
  }

  file.flush();
}

bool VideoRecorder::recordVideo(const char* filename) {
  if (sdManager.exists(filename)) {
    sdManager.deleteFile(filename);
  }

  File aviFile = sdManager.openFile(filename, FILE_WRITE);
  if (!aviFile) {
    Serial.println("Failed to open file for writing");
    return false;
  }

  uint32_t width = 0, height = 0;
  framePixels(FRAME_SIZE, width, height);

  uint32_t estimatedFrames = recordingTimeSeconds * frameRate;
  Serial.printf("Expected to record about %u frames (%d seconds at %d fps)\n",
                estimatedFrames, recordingTimeSeconds, frameRate);

  writeAviHeader(aviFile, width, height, frameRate, estimatedFrames);

  uint32_t maxFrames = estimatedFrames + FRAME_COUNT_HEADROOM;
  uint32_t* frameLengths = (uint32_t*)malloc(maxFrames * sizeof(uint32_t));
  if (!frameLengths) {
    Serial.println("Failed to allocate memory for frame lengths");
    aviFile.close();
    return false;
  }

  unsigned long startTime = millis();
  unsigned long nextFrameTime = startTime;
  unsigned long recordingEndTime = startTime + (recordingTimeSeconds * 1000UL);
  int frameInterval = 1000 / frameRate;

  uint32_t frameCount = 0;
  uint32_t moviPayload = Avi::kFourCC;  // the "movi" FOURCC itself

  Serial.println("Recording started");

  while (millis() < recordingEndTime) {
    unsigned long currentTime = millis();

    if (currentTime < nextFrameTime) {
      delay(1);
      continue;
    }

    if (frameCount >= maxFrames) {
      Serial.println("Frame count exceeds allocated buffer, stopping early");
      break;
    }

    camera_fb_t *fb = captureStableFrame();
    if (!fb) {
      Serial.println("Failed to capture frame");
      nextFrameTime += frameInterval;
      continue;
    }

    if (!writeFrame(aviFile, fb)) {
      Serial.println("SD write failed, aborting recording");
      esp_camera_fb_return(fb);
      free(frameLengths);
      aviFile.close();
      return false;
    }

    frameLengths[frameCount] = fb->len;
    moviPayload += Avi::kChunkPrefix + Avi::paddedLength(fb->len);

    esp_camera_fb_return(fb);
    frameCount++;
    nextFrameTime += frameInterval;

    if (frameCount % FLUSH_INTERVAL_FRAMES == 0) {
      float elapsedSeconds = (currentTime - startTime) / 1000.0;
      Serial.printf("Recorded %u frames, %0.1f seconds, avg FPS: %0.1f (target: %d)\n",
                    frameCount, elapsedSeconds,
                    elapsedSeconds > 0 ? frameCount / elapsedSeconds : 0.0,
                    frameRate);
      aviFile.flush();
    }
  }

  float totalElapsedTime = (millis() - startTime) / 1000.0;
  Serial.printf("Recording complete: %u frames in %0.1f seconds (avg %0.1f FPS)\n",
                frameCount, totalElapsedTime,
                totalElapsedTime > 0 ? frameCount / totalElapsedTime : 0.0);

  // A header-only file is not a recording. Discard it rather than leaving it to
  // be uploaded, which would fill the server with unplayable stubs.
  if (frameCount == 0) {
    Serial.println("No frames captured; discarding empty recording");
    free(frameLengths);
    aviFile.close();
    sdManager.deleteFile(filename);
    return false;
  }

  writeIndex(aviFile, frameCount, frameLengths);
  updateAviHeader(aviFile, frameCount, moviPayload);

  free(frameLengths);
  aviFile.close();

  Serial.println("AVI file finalized");
  return true;
}

void VideoRecorder::analyzeAviFile(const char* filename) {
  File aviFile = sdManager.openFile(filename, FILE_READ);
  if (!aviFile) {
    Serial.println("Failed to open AVI file for analysis");
    return;
  }

  Serial.println("AVI File Analysis:");
  Serial.printf("Filename: %s\n", filename);
  Serial.printf("File size: %u bytes (%.2f MB)\n",
                aviFile.size(), aviFile.size() / 1048576.0);

  char fourcc[5] = {0};
  aviFile.read((uint8_t*)fourcc, 4);
  if (strcmp(fourcc, "RIFF") != 0) {
    Serial.println("Not a valid RIFF file");
    aviFile.close();
    return;
  }

  aviFile.seek(8);
  aviFile.read((uint8_t*)fourcc, 4);
  if (strcmp(fourcc, "AVI ") != 0) {
    Serial.println("Not a valid AVI file");
    aviFile.close();
    return;
  }

  // The avih FOURCC sits 12 bytes into the hdrl LIST, not at the start of the
  // MainAVIHeader data itself.
  aviFile.seek(Avi::kHdrlListOffset + 12);
  aviFile.read((uint8_t*)fourcc, 4);
  if (strcmp(fourcc, "avih") != 0) {
    Serial.println("Could not find avih chunk");
    aviFile.close();
    return;
  }

  uint32_t usPerFrame = 0, frameCount = 0, width = 0, height = 0;
  aviFile.seek(Avi::kMicroSecPerFrame);
  aviFile.read((uint8_t*)&usPerFrame, 4);
  aviFile.seek(Avi::kAvihTotalFrames);
  aviFile.read((uint8_t*)&frameCount, 4);
  aviFile.seek(Avi::kAvihWidth);
  aviFile.read((uint8_t*)&width, 4);
  aviFile.seek(Avi::kAvihHeight);
  aviFile.read((uint8_t*)&height, 4);

  float fps = usPerFrame ? 1000000.0 / usPerFrame : 0.0;

  Serial.printf("Resolution: %u x %u\n", width, height);
  Serial.printf("FPS: %.2f\n", fps);
  Serial.printf("Frame count: %u\n", frameCount);
  Serial.printf("Duration: %.2f seconds\n", fps > 0 ? frameCount / fps : 0.0);

  aviFile.close();
  Serial.println("AVI file analysis complete");
}
