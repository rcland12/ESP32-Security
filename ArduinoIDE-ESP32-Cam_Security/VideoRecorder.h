#ifndef VIDEO_RECORDER_H
#define VIDEO_RECORDER_H

#include "esp_camera.h"
#include "AviFormat.h"
#include "ConfigSettings.h"
#include "PinDefinitions.h"
#include <SDCardManager.h>

class VideoRecorder {
private:
  SDCardManager& sdManager;
  int frameRate;
  int recordingTimeSeconds;
  bool cameraReady = false;

  bool startCamera(pixformat_t format, framesize_t size, int fbCount);
  camera_fb_t* captureStableFrame();

  // The container layout itself lives in AviFormat.h, which is host-testable.
  // These only move bytes onto the card.
  void writeAviHeader(File &file, uint32_t width, uint32_t height,
                      uint32_t fps, uint32_t frameCount);
  void patchU32(File &file, uint32_t offset, uint32_t value);
  void updateAviHeader(File &file, uint32_t totalFrames, uint32_t moviPayload);
  bool writeFrame(File &file, camera_fb_t *fb);
  void writeIndex(File &file, uint32_t frameCount, const uint32_t *frameLengths);

  static void framePixels(framesize_t size, uint32_t &width, uint32_t &height);

public:
  VideoRecorder(SDCardManager& sdManager, int frameRate = FRAME_RATE,
                int recordingTimeSeconds = RECORD_TIME_SEC);

  // Full-quality JPEG mode, used for recording.
  bool initCamera();

  // Low-resolution grayscale mode, used only to decide whether a wake was a
  // real event. Far cheaper to start than JPEG mode, which matters because
  // every false trigger pays this cost and nothing else.
  bool initCameraForDetection();

  void deinitCamera();
  bool isCameraReady() const { return cameraReady; }

  // Reduces the current view to a coarse luminance grid. Requires detection
  // mode. `meanLuminance` reports overall brightness so the caller can tell a
  // dark scene (where comparison is meaningless) from a static one.
  bool captureSignature(uint8_t* signature, uint32_t& meanLuminance);

  // Mean absolute difference across the whole grid.
  static uint32_t signatureDifference(const uint8_t* a, const uint8_t* b);

  // Number of cells that changed by more than `cellDelta`. Far more sensitive
  // to localised motion than the mean, which dilutes a person across 192 cells.
  static uint32_t changedCellCount(const uint8_t* a, const uint8_t* b, uint32_t cellDelta);

  bool recordVideo(const char* filename);
  void analyzeAviFile(const char* filename);
};

#endif
