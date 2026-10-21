#include "rfc2435.h"

#include <cstring>

namespace esphome {
namespace esp_rtsp_stream {

// RFC 2435 Appendix A base tables, in zig-zag order (the order DQT expects).
static const uint8_t LUMA_QUANTIZER[64] = {
    16, 11, 12,  14,  12,  10,  16,  14,  13,  14,  18, 17,  16,  19,  24,  40,
    26, 24, 22,  22,  24,  49,  35,  37,  29,  40,  58, 51,  61,  60,  57,  51,
    56, 55, 64,  72,  92,  78,  64,  68,  87,  69,  55, 56,  80,  109, 81,  87,
    95, 98, 103, 104, 103, 62,  77,  113, 121, 112, 100, 120, 92, 101, 103, 99,
};
static const uint8_t CHROMA_QUANTIZER[64] = {
    17, 18, 18, 24, 21, 24, 47, 26, 26, 47, 99, 66, 56, 66, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
};

// ITU-T T.81 Annex K standard Huffman tables as four DHT segments (DC0, AC0, DC1, AC1).
// Byte data taken from espp/rtsp jpeg_header.hpp (MIT licence).
static const uint8_t STANDARD_DHT[432] = {
    0xff, 0xc4, 0x00, 0x1f, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
    0x0b, 0xff, 0xc4, 0x00, 0xb5, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 0x05, 0x05,
    0x04, 0x04, 0x00, 0x00, 0x01, 0x7d, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31,
    0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42,
    0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18,
    0x19, 0x1a, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43,
    0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63,
    0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83,
    0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a,
    0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8,
    0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6,
    0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2,
    0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xff, 0xc4, 0x00, 0x1f, 0x01, 0x00, 0x03, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
    0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0xff, 0xc4, 0x00, 0xb5, 0x11, 0x00, 0x02,
    0x01, 0x02, 0x04, 0x04, 0x03, 0x04, 0x07, 0x05, 0x04, 0x04, 0x00, 0x01, 0x02, 0x77, 0x00, 0x01,
    0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22,
    0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0, 0x15, 0x62,
    0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a,
    0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a,
    0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
    0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe2, 0xe3,
    0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa,
};

static constexpr size_t DQT_SEGMENT_SIZE = 2 + 2 + 1 + 64;
static constexpr size_t DRI_SEGMENT_SIZE = 6;
static constexpr size_t SOF0_SEGMENT_SIZE = 19;
static constexpr size_t SOS_SEGMENT_SIZE = 14;
static constexpr size_t EOI_SIZE = 2;

void MjpegAssembler::set_buffer(uint8_t *buf, size_t capacity) {
  this->buf_ = buf;
  this->capacity_ = capacity;
  this->in_frame_ = false;
}

void MjpegAssembler::make_tables_(int q) {
  const int factor = q < 1 ? 1 : (q > 99 ? 99 : q);
  const int scale = q < 50 ? 5000 / factor : 200 - factor * 2;
  for (int i = 0; i < 64; i++) {
    int lq = (LUMA_QUANTIZER[i] * scale + 50) / 100;
    int cq = (CHROMA_QUANTIZER[i] * scale + 50) / 100;
    this->tables_[i] = lq < 1 ? 1 : (lq > 255 ? 255 : lq);
    this->tables_[64 + i] = cq < 1 ? 1 : (cq > 255 ? 255 : cq);
  }
}

bool MjpegAssembler::begin_frame(const JpegPayloadHeader &hdr, uint16_t restart_interval, const uint8_t *qtables,
                                 size_t qtables_len, uint8_t qtables_precision) {
  this->in_frame_ = false;
  this->next_offset_ = 0;
  this->last_target_ = nullptr;

  const uint8_t base_type = (hdr.type >= 64 && hdr.type < 128) ? hdr.type - 64 : hdr.type;
  if (base_type > 1) {
    this->error_ = "unsupported RTP/JPEG type (only 0, 1, 64 and 65 are defined)";
    return false;
  }
  if (hdr.width == 0 || hdr.height == 0) {
    this->error_ = "image wider or taller than 2040 px cannot be carried by RTP/JPEG";
    return false;
  }

  if (hdr.q == 0 || (hdr.q >= 100 && hdr.q < 128)) {
    this->error_ = "reserved Q value";
    return false;
  }
  if (hdr.q < 128) {
    if (!this->have_tables_ || this->cached_q_ != hdr.q) {
      this->make_tables_(hdr.q);
      this->cached_q_ = hdr.q;
      this->have_tables_ = true;
    }
  } else if (qtables_len > 0) {
    if (qtables_precision != 0) {
      this->error_ = "16-bit quantisation tables are not supported";
      return false;
    }
    if (qtables_len == 128) {
      memcpy(this->tables_, qtables, 128);
    } else if (qtables_len == 64) {
      memcpy(this->tables_, qtables, 64);
      memcpy(this->tables_ + 64, qtables, 64);
    } else {
      this->error_ = "unexpected quantisation table length";
      return false;
    }
    this->cached_q_ = hdr.q;
    this->have_tables_ = true;
  } else if (!this->have_tables_ || this->cached_q_ != hdr.q) {
    this->error_ = "Q >= 128 frame arrived without quantisation tables";
    return false;
  }

  this->header_len_ = this->write_header_(base_type, hdr.width, hdr.height, restart_interval);
  if (this->header_len_ == 0) {
    this->error_ = "frame buffer too small for the JPEG header";
    return false;
  }
  this->width_ = hdr.width;
  this->height_ = hdr.height;
  this->in_frame_ = true;
  return true;
}

size_t MjpegAssembler::write_header_(uint8_t type, uint16_t width, uint16_t height, uint16_t restart_interval) {
  const size_t needed = 2 + 2 * DQT_SEGMENT_SIZE + (restart_interval ? DRI_SEGMENT_SIZE : 0) + SOF0_SEGMENT_SIZE +
                        sizeof(STANDARD_DHT) + SOS_SEGMENT_SIZE;
  if (this->buf_ == nullptr || needed + EOI_SIZE > this->capacity_)
    return 0;

  uint8_t *p = this->buf_;
  *p++ = 0xFF;
  *p++ = 0xD8;  // SOI

  for (uint8_t table = 0; table < 2; table++) {
    *p++ = 0xFF;
    *p++ = 0xDB;  // DQT
    *p++ = 0x00;
    *p++ = 67;
    *p++ = table;
    memcpy(p, this->tables_ + 64 * table, 64);
    p += 64;
  }

  if (restart_interval) {
    *p++ = 0xFF;
    *p++ = 0xDD;  // DRI
    *p++ = 0x00;
    *p++ = 0x04;
    *p++ = restart_interval >> 8;
    *p++ = restart_interval & 0xFF;
  }

  const uint8_t sof0[SOF0_SEGMENT_SIZE] = {
      0xFF, 0xC0, 0x00, 17, 8,  // SOF0, length, 8-bit precision
      static_cast<uint8_t>(height >> 8), static_cast<uint8_t>(height & 0xFF),
      static_cast<uint8_t>(width >> 8),  static_cast<uint8_t>(width & 0xFF),
      3,                                                     // components
      1, static_cast<uint8_t>(type == 0 ? 0x21 : 0x22), 0,  // Y: 2x1 for 4:2:2, 2x2 for 4:2:0
      2, 0x11, 1,                                            // Cb
      3, 0x11, 1,                                            // Cr
  };
  memcpy(p, sof0, sizeof(sof0));
  p += sizeof(sof0);

  memcpy(p, STANDARD_DHT, sizeof(STANDARD_DHT));
  p += sizeof(STANDARD_DHT);

  static const uint8_t SOS[SOS_SEGMENT_SIZE] = {0xFF, 0xDA, 0x00, 12, 3, 1, 0x00, 2, 0x11, 3, 0x11, 0, 63, 0};
  memcpy(p, SOS, sizeof(SOS));
  p += sizeof(SOS);

  return p - this->buf_;
}

uint8_t *MjpegAssembler::fragment_target(uint32_t offset, size_t len) {
  if (!this->in_frame_)
    return nullptr;
  if (offset != this->next_offset_) {
    this->error_ = "missing fragment";
    this->in_frame_ = false;
    return nullptr;
  }
  if (this->header_len_ + offset + len + EOI_SIZE > this->capacity_) {
    this->error_ = "frame larger than max_frame_size";
    this->in_frame_ = false;
    return nullptr;
  }
  this->last_target_ = this->buf_ + this->header_len_ + offset;
  return this->last_target_;
}

void MjpegAssembler::fragment_written(size_t len) {
  if (!this->in_frame_ || this->last_target_ == nullptr)
    return;
  // A payload that starts with SOI is a complete JPEG, not RFC 2435 scan data: drop our header.
  if (this->next_offset_ == 0 && len >= 2 && this->last_target_[0] == 0xFF && this->last_target_[1] == 0xD8) {
    memmove(this->buf_, this->last_target_, len);
    this->header_len_ = 0;
  }
  this->next_offset_ += len;
  this->last_target_ = nullptr;
}

size_t MjpegAssembler::end_frame() {
  if (!this->in_frame_)
    return 0;
  this->in_frame_ = false;
  size_t total = this->header_len_ + this->next_offset_;
  if (this->next_offset_ == 0)
    return 0;
  if (this->buf_[total - 2] != 0xFF || this->buf_[total - 1] != 0xD9) {
    this->buf_[total++] = 0xFF;
    this->buf_[total++] = 0xD9;  // EOI, room reserved by fragment_target()
  }
  return total;
}

}  // namespace esp_rtsp_stream
}  // namespace esphome
