#include "VideoRecorder.h"

VideoRecorder::VideoRecorder(SDCardManager& sdManager, int frameRate, int recordingTimeSeconds)
  : _sdManager(sdManager),
    frameRate(frameRate),
    recordingTimeSeconds(recordingTimeSeconds),
    frameDelayMs(1000 / frameRate) {
}

bool VideoRecorder::initCamera() {
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }

  return true;
}

// void VideoRecorder::writeInt(File &file, uint32_t value) {
//   file.write((uint8_t*)&value, 4);
//   Serial.printf("Wrote value: 0x%08X\n", value);
// }

bool VideoRecorder::recordVideo(const char* filename) {
  File videoFile = _sdManager.openFile(filename, FILE_WRITE);
  if (!videoFile) {
      Serial.println("Failed to open file for writing");
      return false;
  }

  Serial.println("Starting video recording...");
  
  // Reserve space for RIFF header (we'll update it later)
  uint32_t riffOffset = videoFile.position();
  _sdManager.writeInt(videoFile, FOURCC_RIFF);
  _sdManager.writeInt(videoFile, 0);  // File size (to be updated)
  _sdManager.writeInt(videoFile, FOURCC_AVI);

  // Write LIST hdrl
  uint32_t hdrlOffset = videoFile.position();
  _sdManager.writeInt(videoFile, FOURCC_LIST);
  _sdManager.writeInt(videoFile, 192);  // Size of hdrl
  _sdManager.writeInt(videoFile, FOURCC_HDRL);

  // Get camera parameters
  sensor_t * s = esp_camera_sensor_get();
  int width = s->status.framesize > FRAMESIZE_CIF ? 640 : 352;
  int height = s->status.framesize > FRAMESIZE_CIF ? 480 : 288;
  
  Serial.printf("Frame size: %dx%d\n", width, height);

  // Write main AVI header
  _sdManager.writeInt(videoFile, FOURCC_AVIH);
  _sdManager.writeInt(videoFile, sizeof(MainAVIHeader));
  
  MainAVIHeader avih = {0};
  avih.dwMicroSecPerFrame = 1000000 / frameRate;
  avih.dwMaxBytesPerSec = 0;  // Will update this later
  avih.dwPaddingGranularity = 0;
  avih.dwFlags = 0x10;  // AVIF_HASINDEX
  avih.dwTotalFrames = 0;  // Will update this later
  avih.dwInitialFrames = 0;
  avih.dwStreams = 1;
  avih.dwSuggestedBufferSize = 0;  // Will update this later
  avih.dwWidth = width;
  avih.dwHeight = height;
  videoFile.write((uint8_t*)&avih, sizeof(avih));

  // Write stream list
  _sdManager.writeInt(videoFile, FOURCC_LIST);
  _sdManager.writeInt(videoFile, 116);  // Size of strl
  _sdManager.writeInt(videoFile, FOURCC_STRL);

  // Write stream header
  _sdManager.writeInt(videoFile, FOURCC_STRH);
  _sdManager.writeInt(videoFile, sizeof(AVIStreamHeader));
  
  AVIStreamHeader strh = {0};
  strh.fccType = MAKE_FOURCC('v','i','d','s');
  strh.fccHandler = MAKE_FOURCC('M','J','P','G');
  strh.dwFlags = 0;
  strh.wPriority = 0;
  strh.wLanguage = 0;
  strh.dwInitialFrames = 0;
  strh.dwScale = 1;
  strh.dwRate = frameRate;
  strh.dwStart = 0;
  strh.dwLength = 0;  // Will update this later
  strh.dwSuggestedBufferSize = 0;  // Will update this later
  strh.dwQuality = -1;
  strh.dwSampleSize = 0;
  videoFile.write((uint8_t*)&strh, sizeof(strh));

  // Write stream format
  _sdManager.writeInt(videoFile, FOURCC_STRF);
  _sdManager.writeInt(videoFile, sizeof(BITMAPINFOHEADER));
  
  BITMAPINFOHEADER bih = {0};
  bih.biSize = sizeof(BITMAPINFOHEADER);
  bih.biWidth = width;
  bih.biHeight = height;
  bih.biPlanes = 1;
  bih.biBitCount = 24;
  bih.biCompression = MAKE_FOURCC('M','J','P','G');
  bih.biSizeImage = width * height * 3;
  videoFile.write((uint8_t*)&bih, sizeof(bih));

  // Write LIST movi
  uint32_t moviOffset = videoFile.position();
  _sdManager.writeInt(videoFile, FOURCC_LIST);
  _sdManager.writeInt(videoFile, 0);  // Size of movi (to be updated)
  _sdManager.writeInt(videoFile, FOURCC_MOVI);

  uint32_t framesWritten = 0;
  uint32_t maxFrameSize = 0;
  uint32_t totalBytesWritten = 0;
  uint32_t dataStart = videoFile.position();

  // Write frames
  for (int i = 0; i < recordingTimeSeconds * frameRate; i++) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      break;
    }

    _sdManager.writeInt(videoFile, FOURCC_00DC);
    _sdManager.writeInt(videoFile, fb->len);
    videoFile.write(fb->buf, fb->len);
    
    totalBytesWritten += fb->len + 8;  // Include chunk header size
    maxFrameSize = max(maxFrameSize, fb->len);
    framesWritten++;
    
    if (i % 10 == 0) {
      Serial.printf("Written frame %d, size: %d bytes\n", i+1, fb->len);
    }

    esp_camera_fb_return(fb);
    delay(frameDelayMs);
  }

  // Update RIFF header with final size
  uint32_t currentPos = videoFile.position();
  uint32_t moviSize = currentPos - dataStart;
  
  // Update movi LIST size
  videoFile.seek(moviOffset + 4);
  _sdManager.writeInt(videoFile, moviSize + 4);  // Include 'movi' FOURCC
  
  // Update RIFF size
  videoFile.seek(riffOffset + 4);
  _sdManager.writeInt(videoFile, currentPos - riffOffset - 8);  // Entire file minus RIFF header
  
  // Update AVI header with final values
  videoFile.seek(hdrlOffset + 32);  // Position of MainAVIHeader
  avih.dwMaxBytesPerSec = maxFrameSize * frameRate;
  avih.dwTotalFrames = framesWritten;
  avih.dwSuggestedBufferSize = maxFrameSize;
  videoFile.write((uint8_t*)&avih, sizeof(avih));
  
  // Update stream header
  uint32_t strhOffset = hdrlOffset + 32 + sizeof(MainAVIHeader) + 12;
  videoFile.seek(strhOffset);
  strh.dwLength = framesWritten;
  strh.dwSuggestedBufferSize = maxFrameSize;
  videoFile.write((uint8_t*)&strh, sizeof(strh));

  videoFile.close();
  
  Serial.println("\nRecording completed!");
  Serial.printf("Frames written: %d\n", framesWritten);
  Serial.printf("Total data size: %d bytes\n", totalBytesWritten);
  Serial.printf("Max frame size: %d bytes\n", maxFrameSize);
  Serial.printf("Final file size: %d bytes\n", currentPos);

  return true;
}

bool VideoRecorder::analyzeAviFile(const char* filename) {
  File file = SD_MMC.open(filename, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for analysis");
    return false;
  }

  Serial.printf("\nAnalyzing file: %s\n", filename);
  Serial.printf("Total file size: %d bytes\n", file.size());

  uint32_t fourcc;
  uint32_t fileSize;
  uint32_t format;

  file.read((uint8_t*)&fourcc, 4);
  file.read((uint8_t*)&fileSize, 4);
  file.read((uint8_t*)&format, 4);

  Serial.println("\nFile Header Analysis:");
  Serial.printf("FOURCC: %c%c%c%c\n", 
    (char)(fourcc & 0xFF),
    (char)((fourcc >> 8) & 0xFF),
    (char)((fourcc >> 16) & 0xFF),
    (char)((fourcc >> 24) & 0xFF));
  Serial.printf("File Size: %d bytes\n", fileSize + 8);
  Serial.printf("Format: %c%c%c%c\n",
    (char)(format & 0xFF),
    (char)((format >> 8) & 0xFF),
    (char)((format >> 16) & 0xFF),
    (char)((format >> 24) & 0xFF));

  file.seek(0);
  uint32_t frameCount = 0;
  uint32_t lastPosition = 0;
  uint32_t value;

  while (file.position() < file.size() - 4) {
    file.read((uint8_t*)&value, 4);
    if (value == FOURCC_00DC) {
      frameCount++;
      lastPosition = file.position();

      uint32_t frameSize;
      file.read((uint8_t*)&frameSize, 4);
      Serial.printf("Frame %d at position %d, size: %d bytes\n", frameCount, lastPosition, frameSize);

      file.seek(file.position() + frameSize);
    }
  }

  Serial.printf("\nFound %d frames\n", frameCount);
  Serial.printf("Last frame position: %d\n", lastPosition);

  file.close();
  return true;
}