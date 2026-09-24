#ifndef AVI_FORMAT_H
#define AVI_FORMAT_H

#include <stdint.h>
#include <string.h>

// Freestanding description of the RIFF/AVI container this project writes.
//
// Deliberately free of Arduino/ESP dependencies so that test/avi_format_test.cpp
// can compile it on the host and validate real output with ffprobe. That test is
// what keeps the patch offsets below honest.
//
// Layout of a finished file:
//
//   RIFF....AVI            12 bytes
//   LIST hdrl              kHdrlPayload + 8
//     avih                   MainAVIHeader
//     LIST strl
//       strh                 AVIStreamHeader
//       strf                 BITMAPINFOHEADER
//   LIST movi              12 bytes of prologue, then the frames
//     00dc <len> <jpeg>      one per frame, pad byte if len is odd
//   idx1                   16 bytes per frame
//
// Frame count and the two size fields are unknown while recording, so they are
// written as estimates and patched afterwards at the offsets defined here.
namespace Avi {

// --- Chunk payload sizes, excluding each chunk's 8-byte FOURCC + size prefix.
constexpr uint32_t kFourCC      = 4;
constexpr uint32_t kChunkPrefix = 8;

constexpr uint32_t kAvihPayload = 56;  // MainAVIHeader: 14 dwords
constexpr uint32_t kStrhPayload = 56;  // fccType + fccHandler + 10 dwords + rcFrame
constexpr uint32_t kStrfPayload = 40;  // BITMAPINFOHEADER

// LIST strl = "strl" + strh chunk + strf chunk
constexpr uint32_t kStrlPayload = kFourCC
                                + kChunkPrefix + kStrhPayload
                                + kChunkPrefix + kStrfPayload;

// LIST hdrl = "hdrl" + avih chunk + LIST strl
constexpr uint32_t kHdrlPayload = kFourCC
                                + kChunkPrefix + kAvihPayload
                                + kChunkPrefix + kStrlPayload;

// --- Absolute file offsets, derived from the sizes above rather than counted
//     by hand. Every one of these is a seek target for updateHeader().
constexpr uint32_t kRiffSizeOffset = 4;

constexpr uint32_t kHdrlListOffset = 12;
constexpr uint32_t kAvihDataOffset = kHdrlListOffset + kChunkPrefix
                                   + kFourCC + kChunkPrefix;

constexpr uint32_t kMicroSecPerFrame = kAvihDataOffset + 0 * 4;
constexpr uint32_t kMaxBytesPerSec   = kAvihDataOffset + 1 * 4;
constexpr uint32_t kAvihTotalFrames  = kAvihDataOffset + 4 * 4;
constexpr uint32_t kAvihWidth        = kAvihDataOffset + 8 * 4;
constexpr uint32_t kAvihHeight       = kAvihDataOffset + 9 * 4;

constexpr uint32_t kStrlListOffset = kHdrlListOffset + kChunkPrefix
                                   + kFourCC + kChunkPrefix + kAvihPayload;
constexpr uint32_t kStrhDataOffset = kStrlListOffset + kChunkPrefix
                                   + kFourCC + kChunkPrefix;

// Dwords inside AVIStreamHeader start after fccType + fccHandler.
constexpr uint32_t kStrhFields = kStrhDataOffset + 8;
constexpr uint32_t kStrhScale  = kStrhFields + 3 * 4;
constexpr uint32_t kStrhRate   = kStrhFields + 4 * 4;
constexpr uint32_t kStrhLength = kStrhFields + 6 * 4;

constexpr uint32_t kStrfOffset     = kStrlListOffset + kChunkPrefix + kFourCC
                                   + kChunkPrefix + kStrhPayload;
constexpr uint32_t kMoviListOffset = kStrfOffset + kChunkPrefix + kStrfPayload;
constexpr uint32_t kMoviSizeOffset = kMoviListOffset + kFourCC;
constexpr uint32_t kHeaderSize     = kMoviListOffset + kChunkPrefix + kFourCC;

// idx1 offsets are measured from the "movi" FOURCC, so the first frame's
// 00dc chunk sits at 4.
constexpr uint32_t kFirstFrameIndexOffset = kFourCC;
constexpr uint32_t kIndexEntrySize        = 16;

// Tripwires. If a payload size above is ever edited, these fire at compile time
// and force the patch offsets to be re-derived, rather than every recording
// silently corrupting.
static_assert(kStrlPayload == 116, "strl LIST payload no longer matches the AVI spec");
static_assert(kHdrlPayload == 192, "hdrl LIST payload no longer matches the AVI spec");
static_assert(kStrhDataOffset == 108, "strh moved; re-derive the strh patch offsets");
static_assert(kMoviSizeOffset == 216, "movi size patch offset moved");
static_assert(kHeaderSize == 224, "AVI header size changed; re-verify every offset");

// --- Little-endian scalar writers. AVI is little-endian regardless of host.
inline void putU16(uint8_t* p, uint16_t v) {
  p[0] = static_cast<uint8_t>(v & 0xFF);
  p[1] = static_cast<uint8_t>(v >> 8);
}

inline void putU32(uint8_t* p, uint32_t v) {
  p[0] = static_cast<uint8_t>(v & 0xFF);
  p[1] = static_cast<uint8_t>((v >> 8) & 0xFF);
  p[2] = static_cast<uint8_t>((v >> 16) & 0xFF);
  p[3] = static_cast<uint8_t>((v >> 24) & 0xFF);
}

inline void putFourCC(uint8_t* p, const char* cc) { memcpy(p, cc, kFourCC); }

// AVI chunks are word-aligned: an odd-length frame gains a single pad byte.
// The pad counts toward file offsets but never toward the reported chunk length.
inline uint32_t paddedLength(uint32_t length) { return length + (length & 1u); }

// Fills `out` (kHeaderSize bytes) with the header prologue, ending at the "movi"
// FOURCC so that frame data can be appended directly afterwards.
inline void buildHeader(uint8_t* out, uint32_t width, uint32_t height,
                        uint32_t fps, uint32_t frameCount) {
  memset(out, 0, kHeaderSize);

  putFourCC(out, "RIFF");
  putU32(out + kRiffSizeOffset, 0);  // patched on completion
  putFourCC(out + 8, "AVI ");

  putFourCC(out + kHdrlListOffset, "LIST");
  putU32(out + kHdrlListOffset + 4, kHdrlPayload);
  putFourCC(out + kHdrlListOffset + 8, "hdrl");

  putFourCC(out + kHdrlListOffset + 12, "avih");
  putU32(out + kHdrlListOffset + 16, kAvihPayload);

  uint8_t* avih = out + kAvihDataOffset;
  putU32(avih + 0 * 4, fps ? 1000000u / fps : 0);  // dwMicroSecPerFrame
  putU32(avih + 1 * 4, width * height * fps);      // dwMaxBytesPerSec (estimate)
  putU32(avih + 2 * 4, 0);                         // dwPaddingGranularity
  putU32(avih + 3 * 4, 0x10);                      // dwFlags = AVIF_HASINDEX
  putU32(avih + 4 * 4, frameCount);                // dwTotalFrames
  putU32(avih + 5 * 4, 0);                         // dwInitialFrames
  putU32(avih + 6 * 4, 1);                         // dwStreams
  putU32(avih + 7 * 4, 0);                         // dwSuggestedBufferSize
  putU32(avih + 8 * 4, width);                     // dwWidth
  putU32(avih + 9 * 4, height);                    // dwHeight
  // dwReserved[4] stays zero

  putFourCC(out + kStrlListOffset, "LIST");
  putU32(out + kStrlListOffset + 4, kStrlPayload);
  putFourCC(out + kStrlListOffset + 8, "strl");

  putFourCC(out + kStrlListOffset + 12, "strh");
  putU32(out + kStrlListOffset + 16, kStrhPayload);

  uint8_t* strh = out + kStrhDataOffset;
  putFourCC(strh + 0, "vids");                      // fccType
  putFourCC(strh + 4, "MJPG");                      // fccHandler
  uint8_t* f = out + kStrhFields;
  putU32(f + 0 * 4, 0);                             // dwFlags
  putU32(f + 1 * 4, 0);                             // wPriority + wLanguage
  putU32(f + 2 * 4, 0);                             // dwInitialFrames
  putU32(f + 3 * 4, 1);                             // dwScale
  putU32(f + 4 * 4, fps);                           // dwRate; fps = rate / scale
  putU32(f + 5 * 4, 0);                             // dwStart
  putU32(f + 6 * 4, frameCount);                    // dwLength
  putU32(f + 7 * 4, 0);                             // dwSuggestedBufferSize
  putU32(f + 8 * 4, 10000);                         // dwQuality (10000 = default)
  putU32(f + 9 * 4, 0);                             // dwSampleSize
  putU16(f + 10 * 4 + 0, 0);                        // rcFrame.left
  putU16(f + 10 * 4 + 2, 0);                        // rcFrame.top
  putU16(f + 10 * 4 + 4, static_cast<uint16_t>(width));   // rcFrame.right
  putU16(f + 10 * 4 + 6, static_cast<uint16_t>(height));  // rcFrame.bottom

  putFourCC(out + kStrfOffset, "strf");
  putU32(out + kStrfOffset + 4, kStrfPayload);

  uint8_t* strf = out + kStrfOffset + kChunkPrefix;
  putU32(strf + 0 * 4, kStrfPayload);           // biSize
  putU32(strf + 1 * 4, width);                  // biWidth
  putU32(strf + 2 * 4, height);                 // biHeight
  putU32(strf + 3 * 4, 1u | (24u << 16));       // biPlanes | biBitCount
  putFourCC(strf + 4 * 4, "MJPG");              // biCompression
  putU32(strf + 5 * 4, width * height * 3);     // biSizeImage
  // biXPelsPerMeter, biYPelsPerMeter, biClrUsed, biClrImportant stay zero

  putFourCC(out + kMoviListOffset, "LIST");
  putU32(out + kMoviSizeOffset, 0);  // patched on completion
  putFourCC(out + kMoviListOffset + kChunkPrefix, "movi");
}

// Writes the 8-byte "00dc" prefix preceding one frame's JPEG payload.
inline void buildFrameChunkPrefix(uint8_t* out, uint32_t length) {
  putFourCC(out, "00dc");
  putU32(out + 4, length);
}

// One idx1 entry. `chunkOffset` is relative to the "movi" FOURCC; `length` is
// the true payload length, excluding any pad byte.
inline void buildIndexEntry(uint8_t* out, uint32_t chunkOffset, uint32_t length) {
  putFourCC(out, "00dc");
  putU32(out + 4, 0x10);  // AVIIF_KEYFRAME; every MJPEG frame is a keyframe
  putU32(out + 8, chunkOffset);
  putU32(out + 12, length);
}

}  // namespace Avi

#endif
