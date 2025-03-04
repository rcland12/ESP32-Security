#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"
#include "VideoRecorder.h"

VideoRecorder::VideoRecorder(SDCardManager& sdManager, int frameRate, int recordingTimeSeconds) 
  : sdManager(sdManager), frameRate(frameRate), recordingTimeSeconds(recordingTimeSeconds) {
}

bool VideoRecorder::initCamera() {
  Serial.println("Setting up camera...");

  camera_config_t config;
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

  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAME_SIZE;
  config.jpeg_quality = JPEG_QUALITY;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return false;
  }

  Serial.printf("Camera test capture OK: %d bytes\n", fb->len);
  esp_camera_fb_return(fb);

  sensor_t *s = esp_camera_sensor_get();
  s->set_quality(s, JPEG_QUALITY);
  s->set_framesize(s, FRAME_SIZE);
  s->set_brightness(s, 0);
  s->set_contrast(s, 0);
  s->set_saturation(s, 0);
  s->set_whitebal(s, 1);
  s->set_exposure_ctrl(s, 1);

  Serial.println("Camera setup complete");
  return true;
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

void VideoRecorder::createAviHeader(File &file, int width, int height, int fps, int num_frames) {
  const char riff[] = "RIFF";
  const char avi[] = "AVI ";
  const char list_hdrl[] = "LIST";
  const char hdrl[] = "hdrl";
  const char avih[] = "avih";
  const char list_strl[] = "LIST";
  const char strl[] = "strl";
  const char strh[] = "strh";
  const char vids[] = "vids";
  const char mjpg[] = "MJPG";
  const char strf[] = "strf";
  const char list_movi[] = "LIST";
  const char movi[] = "movi";

  uint32_t us_per_frame = 100000;

  uint32_t max_bytes_per_sec = width * height * 3 * fps;

  file.write((const uint8_t*)riff, 4);
  uint32_t riff_size = 0;
  file.write((uint8_t*)&riff_size, 4);
  file.write((const uint8_t*)avi, 4);

  file.write((const uint8_t*)list_hdrl, 4);
  uint32_t hdrl_size = 192;
  file.write((uint8_t*)&hdrl_size, 4);
  file.write((const uint8_t*)hdrl, 4);

  file.write((const uint8_t*)avih, 4);
  uint32_t avih_size = 56;
  file.write((uint8_t*)&avih_size, 4);

  uint32_t avih_data[14];
  avih_data[0] = us_per_frame;
  avih_data[1] = max_bytes_per_sec;
  avih_data[2] = 0;
  avih_data[3] = 0x10;
  avih_data[4] = num_frames;
  avih_data[5] = 0;
  avih_data[6] = 1;
  avih_data[7] = 0;
  avih_data[8] = width;
  avih_data[9] = height;
  avih_data[10] = 0;
  avih_data[11] = 0;
  avih_data[12] = 0;
  avih_data[13] = 0;
  file.write((uint8_t*)avih_data, 56);

  file.write((const uint8_t*)list_strl, 4);
  uint32_t strl_size = 116;
  file.write((uint8_t*)&strl_size, 4);
  file.write((const uint8_t*)strl, 4);

  file.write((const uint8_t*)strh, 4);
  uint32_t strh_size = 48;
  file.write((uint8_t*)&strh_size, 4);

  file.write((const uint8_t*)vids, 4);
  file.write((const uint8_t*)mjpg, 4);
  uint32_t strh_data[10];
  strh_data[0] = 0;
  strh_data[1] = 0;
  strh_data[2] = 0;

  strh_data[3] = 1;
  strh_data[4] = fps;
  strh_data[5] = 0;
  strh_data[6] = num_frames;
  strh_data[7] = 0;
  strh_data[8] = 10000;
  strh_data[9] = 0;
  file.write((uint8_t*)strh_data, 40);

  file.write((const uint8_t*)strf, 4);
  uint32_t strf_size = 40;
  file.write((uint8_t*)&strf_size, 4);

  uint32_t strf_data[10];
  strf_data[0] = 40;
  strf_data[1] = width;
  strf_data[2] = height;
  strf_data[3] = 1 | (24 << 16);
  memcpy(&strf_data[4], mjpg, 4);
  strf_data[5] = width * height * 3;
  strf_data[6] = 0;
  strf_data[7] = 0;
  strf_data[8] = 0;
  strf_data[9] = 0;
  file.write((uint8_t*)strf_data, 40);

  file.write((const uint8_t*)list_movi, 4);
  uint32_t movi_size = 0;
  file.write((uint8_t*)&movi_size, 4);
  file.write((const uint8_t*)movi, 4);

  file.flush();
}

void VideoRecorder::updateAviHeader(File &file, uint32_t total_frames, uint32_t *frame_sizes, uint32_t movi_size) {
  uint32_t max_frame_size = 0;
  for (uint32_t i = 0; i < total_frames; i++) {
    if (frame_sizes[i] > max_frame_size) {
      max_frame_size = frame_sizes[i];
    }
  }

  uint32_t file_size = file.size();

  uint32_t riff_size = file_size - 8;

  Serial.printf(
    "Updating headers: frames=%d, file_size=%d, riff_size=%d, movi_size=%d\n", 
    total_frames,
    file_size,
    riff_size,
    movi_size
  );

  file.seek(4);
  file.write((uint8_t*)&riff_size, 4);

  file.seek(0x30);
  file.write((uint8_t*)&total_frames, 4);

  file.seek(0x38);
  file.write((uint8_t*)&max_frame_size, 4);

  file.seek(0x84);
  file.write((uint8_t*)&total_frames, 4);

  file.seek(0xFC);
  uint32_t movi_list_size = movi_size + 4;
  file.write((uint8_t*)&movi_list_size, 4);

  file.flush();
}

void maintain_frame_rate(unsigned long &next_frame_time, int frame_interval, int frame_count) {
  unsigned long current_time = millis();
  unsigned long target_time = next_frame_time + frame_interval;

  if (target_time < current_time) {
    if (current_time - target_time > 500) {
      next_frame_time = current_time;
    } else {
      next_frame_time = target_time;
    }
  } else {
    next_frame_time = target_time;
    while (millis() < next_frame_time) {
      delay(1);
    }
  }

  if (frame_count % 30 == 0) {
    Serial.printf(
      "Frame %d, time diff: %ld ms\n", 
      frame_count,
      (long)(millis() - next_frame_time)
    );
  }
}

void VideoRecorder::writeQuartet(uint32_t value, File &file) {
  uint8_t buf[4];
  buf[0] = value & 0xFF;
  buf[1] = (value >> 8) & 0xFF;
  buf[2] = (value >> 16) & 0xFF;
  buf[3] = (value >> 24) & 0xFF;
  file.write(buf, 4);
}

void VideoRecorder::writeFrame(File &file, camera_fb_t *fb, uint32_t *frame_size) {
  file.write((const uint8_t*)"00dc", 4);

  file.write((uint8_t*)&fb->len, 4);

  file.write(fb->buf, fb->len);

  *frame_size = fb->len;

  if (fb->len % 2 == 1) {
    uint8_t padding = 0;
    file.write(&padding, 1);
    (*frame_size)++;
  }

  file.flush();
}

void VideoRecorder::writeIndex(File &file, uint32_t frame_count, uint32_t *frame_sizes, uint32_t movi_start) {
  file.write((const uint8_t*)"idx1", 4);

  uint32_t idx1_size = frame_count * 16;
  file.write((uint8_t*)&idx1_size, 4);

  uint32_t offset = 4;

  Serial.printf("Writing index with %d entries, movi_start=%d\n", frame_count, movi_start);

  for (uint32_t i = 0; i < frame_count; i++) {
    file.write((const uint8_t*)"00dc", 4);

    uint32_t flags = 0x10;
    file.write((uint8_t*)&flags, 4);

    file.write((uint8_t*)&offset, 4);

    file.write((uint8_t*)&frame_sizes[i], 4);

    offset += 8 + frame_sizes[i];

    if (i % 10 == 0) {
      Serial.printf("Index entry %d: offset=%d, size=%d\n", i, offset - (8 + frame_sizes[i]), frame_sizes[i]);
    }
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

  int width = 0, height = 0;

  switch (FRAME_SIZE) {
    case FRAMESIZE_QVGA: width = 320; height = 240; break;
    case FRAMESIZE_VGA: width = 640; height = 480; break;
    case FRAMESIZE_SVGA: width = 800; height = 600; break;
    case FRAMESIZE_XGA: width = 1024; height = 768; break;
    case FRAMESIZE_HD: width = 1280; height = 720; break;
    default: width = 640; height = 480;
  }

  uint32_t estimated_frames = RECORD_TIME * FRAME_RATE;
  Serial.printf(
    "Expected to record about %d frames (%d seconds at %d fps)\n", 
    estimated_frames,
    RECORD_TIME,
    FRAME_RATE
  );

  createAviHeader(aviFile, width, height, FRAME_RATE, estimated_frames);

  uint32_t movi_start = aviFile.position() - 4;
  Serial.printf("Movi chunk starts at byte %d\n", movi_start);

  Serial.println("Recording started");

  unsigned long startTime = millis();
  unsigned long nextFrameTime = startTime;
  int frameInterval = 1000 / FRAME_RATE;
  uint32_t frameCount = 0;
  uint32_t moviSize = 0;

  uint32_t max_frames = estimated_frames + 10;
  uint32_t* frameSizes = (uint32_t*)malloc(max_frames * sizeof(uint32_t));
  if (!frameSizes) {
    Serial.println("Failed to allocate memory for frame sizes");
    aviFile.close();
    return false;
  }

  while ((millis() - startTime) < (RECORD_TIME * 1000)) {
    if (millis() >= nextFrameTime) {
      camera_fb_t *fb = captureStableFrame();
      if (!fb) {
        Serial.println("Failed to capture frame");
        nextFrameTime += frameInterval;
        continue;
      }

      if (frameCount >= max_frames) {
        Serial.println("Frame count exceeds allocated buffer!");
        esp_camera_fb_return(fb);
        break;
      }

      uint32_t frameSize = 0;
      writeFrame(aviFile, fb, &frameSize);

      frameSizes[frameCount] = frameSize;

      moviSize += 8 + frameSize;

      esp_camera_fb_return(fb);
      frameCount++;

      maintain_frame_rate(nextFrameTime, frameInterval, frameCount);

      if (frameCount % 10 == 0) {
        Serial.printf(
          "Recorded %d frames, %0.1f seconds, avg FPS: %0.1f\n", 
          frameCount, 
          (millis() - startTime) / 1000.0,
          frameCount / ((millis() - startTime) / 1000.0)
        );
        aviFile.flush();
      }

    } else {
      delay(1);
    }
  }

  Serial.printf(
    "Recording complete: %d frames in %0.1f seconds (avg %0.1f FPS)\n", 
    frameCount, 
    (millis() - startTime) / 1000.0,
    frameCount / ((millis() - startTime) / 1000.0)
  );

  writeIndex(aviFile, frameCount, frameSizes, movi_start);

  updateAviHeader(aviFile, frameCount, frameSizes, moviSize);

  free(frameSizes);

  aviFile.flush();
  aviFile.close();

  Serial.println("AVI file finalized");
  delay(1000);

  Serial.printf(
    "Recording summary: %d frames in %d seconds (target: %d FPS)\n", 
    frameCount,
    RECORD_TIME,
    FRAME_RATE
  );

  return true;
}

void VideoRecorder::analyzeAviFile(const char* filename) {
  File aviFile = sdManager.openFile(filename, FILE_READ);
  if (!aviFile) {
    Serial.println("Failed to open AVI file for analysis");
    return;
  }

  size_t fileSize = aviFile.size();

  Serial.println("AVI File Analysis:");
  Serial.printf("Filename: %s\n", filename);
  Serial.printf("File size: %u bytes (%.2f MB)\n", fileSize, fileSize / 1048576.0);

  char header[5] = {0};
  aviFile.read((uint8_t*)header, 4);
  if (strcmp(header, "RIFF") != 0) {
    Serial.println("Not a valid RIFF file");
    aviFile.close();
    return;
  }

  uint32_t riffSize;
  aviFile.read((uint8_t*)&riffSize, 4);

  aviFile.read((uint8_t*)header, 4);
  if (strcmp(header, "AVI ") != 0) {
    Serial.println("Not a valid AVI file");
    aviFile.close();
    return;
  }

  aviFile.seek(0x20);
  aviFile.read((uint8_t*)header, 4);
  if (strcmp(header, "avih") != 0) {
    Serial.println("Could not find avih chunk");
    aviFile.close();
    return;
  }

  aviFile.seek(aviFile.position() + 4);

  uint32_t usPerFrame;
  aviFile.read((uint8_t*)&usPerFrame, 4);
  float fps = 1000000.0 / usPerFrame;

  aviFile.seek(0x30);
  uint32_t frameCount;
  aviFile.read((uint8_t*)&frameCount, 4);

  aviFile.seek(0x40);
  uint32_t width, height;
  aviFile.read((uint8_t*)&width, 4);
  aviFile.read((uint8_t*)&height, 4);

  Serial.printf("Resolution: %u x %u\n", width, height);
  Serial.printf("FPS: %.2f\n", fps);
  Serial.printf("Frame count: %u\n", frameCount);
  Serial.printf("Duration: %.2f seconds\n", frameCount / fps);

  aviFile.close();
  Serial.println("AVI file analysis complete");
}
