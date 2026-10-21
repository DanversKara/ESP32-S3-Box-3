#include "avi_writer.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include <esp_heap_caps.h>

#include "esphome/core/log.h"

namespace esphome {
namespace esp_rtsp_stream {

static const char *const TAG = "esp_rtsp_stream.avi";

// Fixed header layout (offsets in bytes):
//   0 RIFF size 'AVI '
//  12 LIST size 'hdrl'
//  24   'avih' 56          data at 32
//  88   LIST size 'strl'
// 100     'strh' 56        data at 108
// 164     'strf' 40        data at 172 (BITMAPINFOHEADER)
// 212 LIST size 'movi'     first frame chunk at 224
static constexpr uint32_t HEADER_SIZE = 224;
static constexpr uint32_t MOVI_FOURCC_POS = 220;
static constexpr uint32_t AVIF_HASINDEX = 0x10;
static constexpr uint32_t AVIIF_KEYFRAME = 0x10;
static constexpr size_t BUFFER_SIZE = 4096;  // 8 sectors; one per open file

static void put32(uint8_t *p, uint32_t v) {
  p[0] = v;
  p[1] = v >> 8;
  p[2] = v >> 16;
  p[3] = v >> 24;
}
static void put16(uint8_t *p, uint16_t v) {
  p[0] = v;
  p[1] = v >> 8;
}
static void fourcc(uint8_t *p, const char *cc) { memcpy(p, cc, 4); }

/// Builds the whole 224-byte header. Called with zeros on open() and with real values on close().
static void build_header(uint8_t *h, uint32_t riff_size, uint32_t frames, uint32_t usec_per_frame,
                         uint32_t rate_millifps, uint32_t max_bytes_per_sec, uint32_t max_chunk, int width,
                         int height, uint32_t movi_size) {
  memset(h, 0, HEADER_SIZE);
  fourcc(h + 0, "RIFF");
  put32(h + 4, riff_size);
  fourcc(h + 8, "AVI ");

  fourcc(h + 12, "LIST");
  put32(h + 16, 192);  // 'hdrl' + avih chunk (64) + strl LIST (124)
  fourcc(h + 20, "hdrl");

  fourcc(h + 24, "avih");
  put32(h + 28, 56);
  put32(h + 32, usec_per_frame);
  put32(h + 36, max_bytes_per_sec);
  put32(h + 40, 0);              // padding granularity
  put32(h + 44, AVIF_HASINDEX);  // flags
  put32(h + 48, frames);         // total frames
  put32(h + 52, 0);              // initial frames
  put32(h + 56, 1);              // streams
  put32(h + 60, max_chunk);      // suggested buffer size
  put32(h + 64, width);
  put32(h + 68, height);
  // 72..87 reserved

  fourcc(h + 88, "LIST");
  put32(h + 92, 116);  // 'strl' + strh chunk (64) + strf chunk (48)
  fourcc(h + 96, "strl");

  fourcc(h + 100, "strh");
  put32(h + 104, 56);
  fourcc(h + 108, "vids");
  fourcc(h + 112, "MJPG");
  put32(h + 116, 0);              // flags
  put32(h + 120, 0);              // priority, language
  put32(h + 124, 0);              // initial frames
  put32(h + 128, 1000);           // scale
  put32(h + 132, rate_millifps);  // rate (rate/scale = fps)
  put32(h + 136, 0);              // start
  put32(h + 140, frames);         // length
  put32(h + 144, max_chunk);      // suggested buffer size
  put32(h + 148, 0xFFFFFFFF);     // quality: default
  put32(h + 152, 0);              // sample size
  put16(h + 156, 0);              // rcFrame
  put16(h + 158, 0);
  put16(h + 160, width);
  put16(h + 162, height);

  fourcc(h + 164, "strf");
  put32(h + 168, 40);
  put32(h + 172, 40);  // biSize
  put32(h + 176, width);
  put32(h + 180, height);
  put16(h + 184, 1);   // planes
  put16(h + 186, 24);  // bit count
  fourcc(h + 188, "MJPG");
  put32(h + 192, static_cast<uint32_t>(width) * height * 3);
  // 196..211: resolution, colours: 0

  fourcc(h + 212, "LIST");
  put32(h + 216, movi_size);
  fourcc(h + 220, "movi");
}

bool AviWriter::open(const std::string &path, uint32_t max_frames) {
  this->close();
  this->index_ = static_cast<uint32_t *>(heap_caps_malloc(max_frames * 2 * sizeof(uint32_t), MALLOC_CAP_SPIRAM));
  if (this->index_ == nullptr) {
    ESP_LOGE(TAG, "Cannot allocate the index for %u frames", (unsigned) max_frames);
    return false;
  }
  this->buf_ = static_cast<uint8_t *>(heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL));
  if (this->buf_ == nullptr)  // works from PSRAM too, just slower
    this->buf_ = static_cast<uint8_t *>(heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_SPIRAM));
  this->buf_len_ = 0;
  this->fd_ = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (this->fd_ < 0 || this->buf_ == nullptr) {
    ESP_LOGE(TAG, "Cannot create %s (is the SD card mounted?)", path.c_str());
    if (this->fd_ >= 0)
      ::close(this->fd_);
    this->fd_ = -1;
    heap_caps_free(this->index_);
    this->index_ = nullptr;
    if (this->buf_ != nullptr)
      heap_caps_free(this->buf_);
    this->buf_ = nullptr;
    return false;
  }
  this->path_ = path;
  this->index_capacity_ = max_frames;
  this->frames_ = 0;
  this->movi_bytes_ = 4;  // the 'movi' fourcc itself counts towards the LIST size
  this->max_chunk_ = 0;
  this->width_ = this->height_ = 0;
  uint8_t header[HEADER_SIZE];
  build_header(header, 0, 0, 0, 0, 0, 0, 0, 0, 4);
  if (!this->write_(header, sizeof(header))) {
    this->close();
    return false;
  }
  return true;
}

bool AviWriter::flush_buffer_() {
  size_t done = 0;
  while (done < this->buf_len_) {
    const ssize_t n = ::write(this->fd_, this->buf_ + done, this->buf_len_ - done);
    if (n <= 0) {
      ESP_LOGE(TAG, "Write to %s failed (card full or removed?)", this->path_.c_str());
      this->buf_len_ = 0;
      return false;
    }
    done += n;
  }
  this->buf_len_ = 0;
  return true;
}

bool AviWriter::write_(const void *data, size_t len) {
  const uint8_t *src = static_cast<const uint8_t *>(data);
  while (len > 0) {
    const size_t n = std::min(len, BUFFER_SIZE - this->buf_len_);
    memcpy(this->buf_ + this->buf_len_, src, n);
    this->buf_len_ += n;
    src += n;
    len -= n;
    if (this->buf_len_ == BUFFER_SIZE && !this->flush_buffer_())
      return false;
  }
  return true;
}

bool AviWriter::add(const uint8_t *jpeg, size_t len, int width, int height, uint32_t now_ms) {
  if (this->fd_ < 0 || this->full() || len == 0 || len > 0x7FFFFFFF)
    return false;
  if (this->frames_ == 0) {
    this->width_ = width;
    this->height_ = height;
    this->first_ms_ = now_ms;
  }
  this->last_ms_ = now_ms;

  const uint32_t offset = HEADER_SIZE + (this->movi_bytes_ - 4) - (MOVI_FOURCC_POS);  // relative to 'movi'
  uint8_t chunk[8];
  fourcc(chunk, "00dc");
  put32(chunk + 4, len);
  if (!this->write_(chunk, sizeof(chunk)) || !this->write_(jpeg, len))
    return false;
  uint32_t chunk_bytes = 8 + len;
  if (len & 1) {
    const uint8_t pad = 0;
    if (!this->write_(&pad, 1))
      return false;
    chunk_bytes++;
  }
  this->index_[this->frames_ * 2] = offset;
  this->index_[this->frames_ * 2 + 1] = len;
  this->frames_++;
  this->movi_bytes_ += chunk_bytes;
  if (len > this->max_chunk_)
    this->max_chunk_ = len;
  return true;
}

void AviWriter::flush() {
  if (this->fd_ < 0)
    return;
  // Only whole buffers are written here so the file stays 4 KB aligned; fsync commits them.
  fsync(this->fd_);
}

void AviWriter::close() {
  if (this->fd_ >= 0) {
    bool ok = true;
    // idx1: one 16-byte entry per frame.
    uint8_t chunk[8];
    fourcc(chunk, "idx1");
    put32(chunk + 4, this->frames_ * 16);
    ok = this->write_(chunk, sizeof(chunk));
    uint8_t entries[16 * 32];
    uint32_t n = 0;
    for (uint32_t i = 0; ok && i < this->frames_; i++) {
      uint8_t *e = entries + 16 * n;
      fourcc(e, "00dc");
      put32(e + 4, AVIIF_KEYFRAME);
      put32(e + 8, this->index_[i * 2]);
      put32(e + 12, this->index_[i * 2 + 1]);
      if (++n == 32 || i + 1 == this->frames_) {
        ok = this->write_(entries, 16 * n);
        n = 0;
      }
    }

    if (ok) {
      const uint32_t file_size = HEADER_SIZE + (this->movi_bytes_ - 4) + 8 + this->frames_ * 16;
      // Real frame rate from the timestamps, so playback runs at recorded speed.
      const uint32_t span_ms = this->frames_ > 1 ? this->last_ms_ - this->first_ms_ : 0;
      uint32_t usec_per_frame = 200000;
      uint32_t rate_millifps = 5000;
      if (span_ms > 0) {
        usec_per_frame = static_cast<uint32_t>(static_cast<uint64_t>(span_ms) * 1000 / (this->frames_ - 1));
        if (usec_per_frame == 0)
          usec_per_frame = 1;
        rate_millifps = static_cast<uint32_t>(1000000000ULL / usec_per_frame);
      }
      const uint32_t bytes_per_sec =
          static_cast<uint32_t>(static_cast<uint64_t>(this->max_chunk_) * rate_millifps / 1000);
      uint8_t header[HEADER_SIZE];
      build_header(header, file_size - 8, this->frames_, usec_per_frame, rate_millifps, bytes_per_sec,
                   this->max_chunk_, this->width_, this->height_, this->movi_bytes_);
      ok = this->flush_buffer_() && lseek(this->fd_, 0, SEEK_SET) == 0 && this->write_(header, sizeof(header)) &&
           this->flush_buffer_();
    } else {
      this->buf_len_ = 0;
    }
    ::close(this->fd_);
    this->fd_ = -1;
    if (ok) {
      ESP_LOGI(TAG, "Saved %s: %u frames, %u KB", this->path_.c_str(), (unsigned) this->frames_,
               (unsigned) ((this->movi_bytes_ + 1023) / 1024));
    } else {
      ESP_LOGW(TAG, "Closed %s without a complete index", this->path_.c_str());
    }
    if (this->frames_ == 0)
      remove(this->path_.c_str());  // nothing recorded: don't leave empty files behind
  }
  if (this->index_ != nullptr) {
    heap_caps_free(this->index_);
    this->index_ = nullptr;
  }
  if (this->buf_ != nullptr) {
    heap_caps_free(this->buf_);
    this->buf_ = nullptr;
  }
  this->frames_ = 0;
  this->index_capacity_ = 0;
}

}  // namespace esp_rtsp_stream
}  // namespace esphome
