#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace esphome {
namespace esp_rtsp_stream {

/// Writes JPEG frames into a Motion-JPEG AVI file (plays in VLC, mpv, Windows Media Player, browsers via
/// ffmpeg, etc.). The JPEGs are stored as-is, so recording costs no CPU beyond the SD-card write.
///
/// The header is written with placeholder sizes and patched, together with the idx1 index, on close().
/// If power is lost before close(), the frames are still on the card; VLC offers to rebuild the index.
class AviWriter {
 public:
  ~AviWriter() { this->close(); }

  /// Creates the file. `max_frames` sizes the in-PSRAM index; add() refuses frames beyond it.
  bool open(const std::string &path, uint32_t max_frames);
  /// Appends one complete JPEG (FFD8...FFD9). `width`/`height` of the first frame go into the header.
  bool add(const uint8_t *jpeg, size_t len, int width, int height, uint32_t now_ms);
  /// Writes the index, patches the header and closes the file. Safe to call when not open.
  void close();
  /// Commits written data to the card (f_sync), so a power cut loses at most the last few seconds.
  void flush();

  bool is_open() const { return this->fd_ >= 0; }
  bool full() const { return this->frames_ >= this->index_capacity_; }
  uint32_t frames() const { return this->frames_; }
  uint32_t bytes() const { return this->movi_bytes_; }
  uint32_t elapsed_ms(uint32_t now_ms) const { return this->frames_ == 0 ? 0 : now_ms - this->first_ms_; }
  const std::string &path() const { return this->path_; }

 protected:
  /// Buffered write. Frames live in PSRAM, which the SD driver can only copy one 512-byte sector at
  /// a time; staging through an internal DMA-capable buffer lets it write 8 sectors per command.
  bool write_(const void *data, size_t len);
  bool flush_buffer_();

  int fd_{-1};
  uint8_t *buf_{nullptr};
  size_t buf_len_{0};
  std::string path_;
  uint32_t *index_{nullptr};  // offset, size pairs
  uint32_t index_capacity_{0};
  uint32_t frames_{0};
  uint32_t movi_bytes_{0};  // bytes after the 'movi' fourcc: chunk headers, data and padding
  uint32_t max_chunk_{0};
  uint32_t first_ms_{0};
  uint32_t last_ms_{0};
  int width_{0};
  int height_{0};
};

}  // namespace esp_rtsp_stream
}  // namespace esphome
