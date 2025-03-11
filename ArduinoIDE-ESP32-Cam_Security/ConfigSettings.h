#ifndef CONFIG_SETTINGS_H
#define CONFIG_SETTINGS_H

#include "esp_camera.h"

constexpr int RECORD_TIME_SEC =               30;
constexpr int FRAME_RATE =                    10;
constexpr framesize_t FRAME_SIZE = FRAMESIZE_VGA;
constexpr int JPEG_QUALITY =                  10;
constexpr int AVIOFFSET =                    240;

#endif