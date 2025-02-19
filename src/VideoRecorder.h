#ifndef VIDEO_RECORDER_H
#define VIDEO_RECORDER_H

#include <Arduino.h>
#include <esp_camera.h>
#include "SDCardManager.h"

#define MAKE_FOURCC(a,b,c,d) ((uint32_t)(d)<<24 | (uint32_t)(c)<<16 | (uint32_t)(b)<<8 | (uint32_t)(a))

const uint32_t FOURCC_RIFF = MAKE_FOURCC('R','I','F','F');
const uint32_t FOURCC_AVI  = MAKE_FOURCC('A','V','I',' ');
const uint32_t FOURCC_LIST = MAKE_FOURCC('L','I','S','T');
const uint32_t FOURCC_HDRL = MAKE_FOURCC('h','d','r','l');
const uint32_t FOURCC_AVIH = MAKE_FOURCC('a','v','i','h');
const uint32_t FOURCC_STRL = MAKE_FOURCC('s','t','r','l');
const uint32_t FOURCC_STRH = MAKE_FOURCC('s','t','r','h');
const uint32_t FOURCC_STRF = MAKE_FOURCC('s','t','r','f');
const uint32_t FOURCC_MOVI = MAKE_FOURCC('m','o','v','i');
const uint32_t FOURCC_00DC = MAKE_FOURCC('0','0','d','c');

#pragma pack(push, 1)
typedef struct {
    uint32_t dwMicroSecPerFrame;
    uint32_t dwMaxBytesPerSec;
    uint32_t dwPaddingGranularity;
    uint32_t dwFlags;
    uint32_t dwTotalFrames;
    uint32_t dwInitialFrames;
    uint32_t dwStreams;
    uint32_t dwSuggestedBufferSize;
    uint32_t dwWidth;
    uint32_t dwHeight;
    uint32_t dwReserved[4];
} MainAVIHeader;

typedef struct {
    uint32_t fccType;
    uint32_t fccHandler;
    uint32_t dwFlags;
    uint16_t wPriority;
    uint16_t wLanguage;
    uint32_t dwInitialFrames;
    uint32_t dwScale;
    uint32_t dwRate;
    uint32_t dwStart;
    uint32_t dwLength;
    uint32_t dwSuggestedBufferSize;
    uint32_t dwQuality;
    uint32_t dwSampleSize;
    struct {
        short int left;
        short int top;
        short int right;
        short int bottom;
    } rcFrame;
} AVIStreamHeader;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)

class VideoRecorder {
public:
  VideoRecorder(SDCardManager& sdManager, int frameRate = 1, int recordingTimeSeconds = 10);
  bool initCamera();
  bool recordVideo(const char* filename);
  bool analyzeAviFile(const char* filename);

private:
  SDCardManager& _sdManager;
  const int frameRate;
  const int recordingTimeSeconds;
  const int frameDelayMs;

  static const int PWDN_GPIO_NUM = 32;
  static const int RESET_GPIO_NUM = -1;
  static const int XCLK_GPIO_NUM = 0;
  static const int SIOD_GPIO_NUM = 26;
  static const int SIOC_GPIO_NUM = 27;
  static const int Y9_GPIO_NUM = 35;
  static const int Y8_GPIO_NUM = 34;
  static const int Y7_GPIO_NUM = 39;
  static const int Y6_GPIO_NUM = 36;
  static const int Y5_GPIO_NUM = 21;
  static const int Y4_GPIO_NUM = 19;
  static const int Y3_GPIO_NUM = 18;
  static const int Y2_GPIO_NUM = 5;
  static const int VSYNC_GPIO_NUM = 25;
  static const int HREF_GPIO_NUM = 23;
  static const int PCLK_GPIO_NUM = 22;
};

#endif