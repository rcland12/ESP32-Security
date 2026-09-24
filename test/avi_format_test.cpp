// Host-side validation of the AVI muxing layout.
//
// AviFormat.h is deliberately free of Arduino dependencies so it can be
// compiled and exercised here, on a PC, using the exact same code the firmware
// runs. This muxes real JPEGs into an AVI and lets ffprobe/ffmpeg judge the
// result, which catches container bugs that on-device testing tends to hide
// (players are forgiving enough to play a subtly malformed file).
//
//   g++ -std=c++17 -I ../lib/VideoRecorder/src -o avi_test avi_format_test.cpp
//
// See test/README for the full run recipe including frame generation.

#include "AviFormat.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

static std::vector<uint8_t> readFile(const std::string& path) {
  std::vector<uint8_t> data;
  FILE* f = fopen(path.c_str(), "rb");
  if (!f) return data;
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  data.resize(static_cast<size_t>(size));
  if (fread(data.data(), 1, data.size(), f) != data.size()) data.clear();
  fclose(f);
  return data;
}

// Mirrors VideoRecorder::recordVideo(), minus the camera and SD card.
static bool muxAvi(const std::string& outPath,
                   const std::vector<std::vector<uint8_t>>& frames,
                   uint32_t width, uint32_t height, uint32_t fps) {
  FILE* out = fopen(outPath.c_str(), "wb+");
  if (!out) {
    fprintf(stderr, "cannot open %s for writing\n", outPath.c_str());
    return false;
  }

  uint8_t header[Avi::kHeaderSize];
  Avi::buildHeader(header, width, height, fps,
                   static_cast<uint32_t>(frames.size()));
  fwrite(header, 1, sizeof(header), out);

  std::vector<uint32_t> lengths;
  lengths.reserve(frames.size());
  uint32_t moviPayload = Avi::kFourCC;  // the "movi" FOURCC itself

  for (const auto& frame : frames) {
    uint32_t len = static_cast<uint32_t>(frame.size());

    uint8_t prefix[Avi::kChunkPrefix];
    Avi::buildFrameChunkPrefix(prefix, len);
    fwrite(prefix, 1, sizeof(prefix), out);
    fwrite(frame.data(), 1, len, out);
    if (len & 1) {
      const uint8_t pad = 0;
      fwrite(&pad, 1, 1, out);
    }

    lengths.push_back(len);
    moviPayload += Avi::kChunkPrefix + Avi::paddedLength(len);
  }

  // idx1
  uint8_t idxPrefix[Avi::kChunkPrefix];
  Avi::putFourCC(idxPrefix, "idx1");
  Avi::putU32(idxPrefix + 4,
              static_cast<uint32_t>(lengths.size()) * Avi::kIndexEntrySize);
  fwrite(idxPrefix, 1, sizeof(idxPrefix), out);

  uint32_t offset = Avi::kFirstFrameIndexOffset;
  for (uint32_t len : lengths) {
    uint8_t entry[Avi::kIndexEntrySize];
    Avi::buildIndexEntry(entry, offset, len);
    fwrite(entry, 1, sizeof(entry), out);
    offset += Avi::kChunkPrefix + Avi::paddedLength(len);
  }

  // Patch the fields that were unknown while writing.
  fseek(out, 0, SEEK_END);
  uint32_t fileSize = static_cast<uint32_t>(ftell(out));
  uint32_t totalFrames = static_cast<uint32_t>(lengths.size());

  auto patch = [&](uint32_t at, uint32_t value) {
    uint8_t buf[4];
    Avi::putU32(buf, value);
    fseek(out, static_cast<long>(at), SEEK_SET);
    fwrite(buf, 1, sizeof(buf), out);
  };

  uint32_t bytesPerSec = totalFrames ? (moviPayload * fps) / totalFrames : 0;

  patch(Avi::kRiffSizeOffset, fileSize - 8);
  patch(Avi::kMicroSecPerFrame, fps ? 1000000u / fps : 0);
  patch(Avi::kMaxBytesPerSec, bytesPerSec);
  patch(Avi::kAvihTotalFrames, totalFrames);
  patch(Avi::kStrhScale, 1);
  patch(Avi::kStrhRate, fps);
  patch(Avi::kStrhLength, totalFrames);
  patch(Avi::kMoviSizeOffset, moviPayload);

  fclose(out);
  return true;
}

static int failures = 0;

static void expectEq(const char* what, uint32_t got, uint32_t want) {
  if (got != want) {
    printf("  FAIL  %-24s got %u, want %u\n", what, got, want);
    failures++;
  } else {
    printf("  ok    %-24s %u\n", what, got);
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    fprintf(stderr, "usage: %s <out.avi> <frame.jpg> [frame.jpg ...]\n", argv[0]);
    return 2;
  }

  printf("Layout offsets:\n");
  expectEq("hdrl payload", Avi::kHdrlPayload, 192);
  expectEq("strl payload", Avi::kStrlPayload, 116);
  expectEq("avih data", Avi::kAvihDataOffset, 0x20);
  expectEq("avih totalFrames", Avi::kAvihTotalFrames, 0x30);
  expectEq("avih width", Avi::kAvihWidth, 0x40);
  expectEq("strh rate", Avi::kStrhRate, 0x84);
  expectEq("strh length", Avi::kStrhLength, 0x8C);
  expectEq("movi size", Avi::kMoviSizeOffset, 0xD8);
  expectEq("header size", Avi::kHeaderSize, 224);

  // Padding must not leak into the reported chunk length.
  expectEq("paddedLength(100)", Avi::paddedLength(100), 100);
  expectEq("paddedLength(101)", Avi::paddedLength(101), 102);

  std::vector<std::vector<uint8_t>> frames;
  for (int i = 2; i < argc; i++) {
    auto data = readFile(argv[i]);
    if (data.empty()) {
      fprintf(stderr, "could not read frame %s\n", argv[i]);
      return 2;
    }
    frames.push_back(std::move(data));
  }

  printf("\nMuxing %zu frames into %s\n", frames.size(), argv[1]);
  if (!muxAvi(argv[1], frames, 640, 480, 10)) return 2;

  printf("\n%s\n", failures ? "LAYOUT CHECKS FAILED" : "Layout checks passed.");
  return failures ? 1 : 0;
}
