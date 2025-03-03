#ifndef VIDEO_RECORDER_H
#define VIDEO_RECORDER_H

#include "esp_camera.h"
#include "SDCardManager.h"
#include <Arduino.h>

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define FRAME_SIZE        FRAMESIZE_VGA
#define JPEG_QUALITY      10
#define FRAME_RATE        10
#define RECORD_TIME       10
#define LED_PIN            4
#define AVIOFFSET        240

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