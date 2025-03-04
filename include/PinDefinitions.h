#ifndef PIN_DEFINITIONS_H
#define PIN_DEFINITIONS_H

#include "esp_camera.h"

constexpr int SD_CS =                          5;
constexpr int REC_TIME_SEC =                  15;
constexpr int PIN_PIR =                       12;
constexpr int PWDN_GPIO_NUM =                 32;
constexpr int RESET_GPIO_NUM =                -1;
constexpr int XCLK_GPIO_NUM =                  0;
constexpr int SIOD_GPIO_NUM =                 26;
constexpr int SIOC_GPIO_NUM =                 27;
constexpr int Y9_GPIO_NUM =                   35;
constexpr int Y8_GPIO_NUM =                   34;
constexpr int Y7_GPIO_NUM =                   39;
constexpr int Y6_GPIO_NUM =                   36;
constexpr int Y5_GPIO_NUM =                   21;
constexpr int Y4_GPIO_NUM =                   19;
constexpr int Y3_GPIO_NUM =                   18;
constexpr int Y2_GPIO_NUM =                    5;
constexpr int VSYNC_GPIO_NUM =                25;
constexpr int HREF_GPIO_NUM =                 23;
constexpr int PCLK_GPIO_NUM =                 22;
constexpr framesize_t FRAME_SIZE = FRAMESIZE_VGA;
constexpr int JPEG_QUALITY =                  10;
constexpr int FRAME_RATE =                    10;
constexpr int RECORD_TIME =                   10;
constexpr int LED_PIN =                        4;
constexpr int AVIOFFSET =                    240;

#endif