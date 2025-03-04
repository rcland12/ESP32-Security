#ifndef VIDEO_RECORDER_H
#define VIDEO_RECORDER_H

#include "esp_camera.h"
#include "PinDefinitions.h"
#include <Arduino.h>
#include <SDCardManager.h>

class VideoRecorder {
private:
  SDCardManager& sdManager;
  int frameRate;
  int recordingTimeSeconds;

  camera_fb_t* captureStableFrame();
  void createAviHeader(File &file, int width, int height, int fps, int num_frames);
  void updateAviHeader(File &file, uint32_t total_frames, uint32_t *frame_sizes, uint32_t movi_size);
  void writeQuartet(uint32_t value, File &file);
  void writeFrame(File &file, camera_fb_t *fb, uint32_t *frame_size);
  void writeIndex(File &file, uint32_t frame_count, uint32_t *frame_sizes, uint32_t movi_start);

public:
  VideoRecorder(SDCardManager& sdManager, int frameRate = 10, int recordingTimeSeconds = 10);
  bool initCamera();
  bool recordVideo(const char* filename);
  void analyzeAviFile(const char* filename);
};

#endif