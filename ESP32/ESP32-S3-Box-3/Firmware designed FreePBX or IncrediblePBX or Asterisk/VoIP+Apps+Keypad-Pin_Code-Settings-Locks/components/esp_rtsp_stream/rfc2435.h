#pragma once

#include <cstddef>
#include <cstdint>

namespace esphome {
namespace esp_rtsp_stream {

/// The fixed 8-byte RTP/JPEG main header (RFC 2435 section 3.1).
struct JpegPayloadHeader {
  uint8_t type_specific{0};
  uint32_t fragment_offset{0};
  uint8_t type{0};
  uint8_t q{0};
  uint16_t width{0};  // pixels (header value * 8)
  uint16_t height{0};
};

/// Rebuilds baseline JPEG frames from RFC 2435 payloads straight into a caller-owned buffer.
///
/// Handles types 0/1 (4:2:2 / 4:2:0) and 64/65 (with restart markers), Q 1..99 (standard tables)
/// and Q 128..255 (in-band tables, cached when a camera omits them). Payloads that already carry a
/// complete JPEG (SOI at offset 0), as some non-compliant servers send, are passed through unchanged.
class MjpegAssembler {
 public:
  /// Points the assembler at a new frame buffer; any frame in progress is dropped, cached tables are kept.
  void set_buffer(uint8_t *buf, size_t capacity);

  /// Starts a frame from its first fragment. Writes the JPEG header (SOI..SOS) to the buffer.
  bool begin_frame(const JpegPayloadHeader &hdr, uint16_t restart_interval, const uint8_t *qtables, size_t qtables_len,
                   uint8_t qtables_precision);
  /// Where `len` bytes of scan data at `offset` must be written, or nullptr if the fragment cannot be placed
  /// (no frame in progress, gap, or buffer overflow). On nullptr the frame is abandoned.
  uint8_t *fragment_target(uint32_t offset, size_t len);
  /// Records that `len` bytes were written to the pointer returned by the last fragment_target().
  void fragment_written(size_t len);
  /// Closes the frame on the RTP marker bit and returns the complete JPEG length, or 0 if it is unusable.
  size_t end_frame();
  void abandon() { this->in_frame_ = false; }

  bool in_frame() const { return this->in_frame_; }
  uint16_t width() const { return this->width_; }
  uint16_t height() const { return this->height_; }
  /// Why the last begin_frame()/fragment_target() failed.
  const char *last_error() const { return this->error_; }

 protected:
  size_t write_header_(uint8_t type, uint16_t width, uint16_t height, uint16_t restart_interval);
  void make_tables_(int q);

  uint8_t *buf_{nullptr};
  size_t capacity_{0};
  size_t header_len_{0};
  uint32_t next_offset_{0};
  uint8_t *last_target_{nullptr};
  uint16_t width_{0};
  uint16_t height_{0};
  bool in_frame_{false};

  uint8_t tables_[128];
  uint8_t cached_q_{0};
  bool have_tables_{false};
  const char *error_{""};
};

}  // namespace esp_rtsp_stream
}  // namespace esphome
