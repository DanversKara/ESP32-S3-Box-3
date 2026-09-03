#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#include "stb_image_write.h"

#include "lvgl_screenshot.h"

#ifdef USE_ESP_IDF

#include "esphome/core/log.h"
#include "esp_heap_caps.h"
#include <algorithm>
#include <cstring>

namespace esphome {
namespace lvgl_screenshot {

static const char *const TAG = "lvgl_screenshot";

LvglScreenshot *LvglScreenshot::instance_ = nullptr;

// Context passed to stb's JPEG write callback.
struct JpegWriteCtx {
  uint8_t *buf;
  size_t capacity;
  size_t size;
};

// ---------------------------------------------------------------------------
// jpeg_write_cb_()  –  stb calls this repeatedly as it produces JPEG data
// ---------------------------------------------------------------------------
void LvglScreenshot::jpeg_write_cb_(void *ctx, void *data, int size) {
  auto *c = (JpegWriteCtx *) ctx;
  if (size <= 0 || !data)
    return;
  size_t avail = c->capacity - c->size;
  size_t copy = std::min((size_t) size, avail);
  if (copy < (size_t) size) {
    ESP_LOGW(TAG, "JPEG buffer full — truncating %d -> %u bytes", size, (unsigned) copy);
  }
  memcpy(c->buf + c->size, data, copy);
  c->size += copy;
}

// ---------------------------------------------------------------------------
// setup()
// ---------------------------------------------------------------------------
void LvglScreenshot::setup() {
  instance_ = this;

  this->capture_requested_ = xSemaphoreCreateBinary();
  this->capture_done_ = xSemaphoreCreateBinary();
  if (!this->capture_requested_ || !this->capture_done_) {
    ESP_LOGE(TAG, "Failed to create semaphores");
    this->mark_failed();
    return;
  }

  lv_display_t *disp = lv_display_get_default();
  if (!disp) {
    ESP_LOGE(TAG, "No LVGL display found - is lvgl: initialised before this component?");
    this->mark_failed();
    return;
  }

  uint32_t width = (uint32_t) lv_display_get_horizontal_resolution(disp);
  uint32_t height = (uint32_t) lv_display_get_vertical_resolution(disp);

  // JPEG output is always smaller than raw RGB888; 60% of the raw size is
  // generous headroom even at quality 100 for UI-style flat-color content.
  size_t rgb_size = (size_t) width * height * 3u;
  this->jpeg_capacity_ = rgb_size * 6 / 10;
  this->jpeg_buf_ = (uint8_t *) heap_caps_malloc(this->jpeg_capacity_, MALLOC_CAP_SPIRAM);
  if (!this->jpeg_buf_) {
    ESP_LOGE(TAG, "Failed to allocate %u bytes for JPEG buffer in PSRAM", (unsigned) this->jpeg_capacity_);
    this->mark_failed();
    return;
  }

  this->start_server_();
  ESP_LOGI(TAG, "LVGL screenshot server started — http://<device-ip>:%u/screenshot", this->port_);
}

// ---------------------------------------------------------------------------
// start_server_()  –  spin up esp_http_server on the configured port
// ---------------------------------------------------------------------------
void LvglScreenshot::start_server_() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port = this->port_;
  cfg.stack_size = 8192;
  // Use a unique ctrl_port so it doesn't clash with any other httpd instance.
  cfg.ctrl_port = (uint16_t) (this->port_ + 1u);

  if (httpd_start(&this->server_, &cfg) != ESP_OK) {
    ESP_LOGE(TAG, "Failed to start HTTP server on port %u", this->port_);
    this->server_ = nullptr;
    return;
  }

  httpd_uri_t uri = {
      .uri = "/screenshot",
      .method = HTTP_GET,
      .handler = LvglScreenshot::handle_screenshot_,
      .user_ctx = nullptr,
  };
  httpd_register_uri_handler(this->server_, &uri);
}

// ---------------------------------------------------------------------------
// loop()  –  called from the ESPHome main task; safe to touch LVGL here
// ---------------------------------------------------------------------------
void LvglScreenshot::loop() {
  if (xSemaphoreTake(this->capture_requested_, 0) == pdTRUE) {
    this->do_capture_();
    xSemaphoreGive(this->capture_done_);
  }
}

// ---------------------------------------------------------------------------
// do_capture_()  –  snapshot the active screen, then encode it to JPEG
// ---------------------------------------------------------------------------
void LvglScreenshot::do_capture_() {
  lv_obj_t *screen = lv_screen_active();
  if (!screen) {
    ESP_LOGE(TAG, "No active LVGL screen");
    this->jpeg_size_ = 0;
    return;
  }

  // Renders the current widget tree (whatever page is showing) fresh into a
  // new RGB888 buffer, independent of however LVGL's own display buffer is
  // configured (partial vs. full-frame). This is the part that requires
  // LV_USE_SNAPSHOT=1, set via the build flag in __init__.py.
  lv_draw_buf_t *snap = lv_snapshot_take(screen, LV_COLOR_FORMAT_RGB888);
  if (!snap) {
    ESP_LOGE(TAG, "lv_snapshot_take() failed (out of memory?)");
    this->jpeg_size_ = 0;
    return;
  }

  uint32_t width = snap->header.w;
  uint32_t height = snap->header.h;
  uint32_t stride = snap->header.stride;

  JpegWriteCtx ctx = {this->jpeg_buf_, this->jpeg_capacity_, 0};

  if (stride == width * 3u) {
    // Common case: no row padding, buffer is already tightly packed.
    stbi_write_jpg_to_func(LvglScreenshot::jpeg_write_cb_, &ctx, (int) width, (int) height, 3, snap->data,
                            this->quality_);
  } else {
    // LVGL padded each row for alignment - compact into a tightly packed
    // scratch buffer since stb only understands a fixed width*channels stride.
    uint8_t *packed = (uint8_t *) heap_caps_malloc((size_t) width * height * 3u, MALLOC_CAP_SPIRAM);
    if (packed) {
      for (uint32_t y = 0; y < height; y++) {
        memcpy(packed + y * width * 3u, snap->data + y * stride, width * 3u);
      }
      stbi_write_jpg_to_func(LvglScreenshot::jpeg_write_cb_, &ctx, (int) width, (int) height, 3, packed,
                              this->quality_);
      heap_caps_free(packed);
    } else {
      ESP_LOGE(TAG, "Failed to allocate scratch buffer for row-packing");
    }
  }

  lv_draw_buf_destroy(snap);

  this->jpeg_size_ = ctx.size;
  ESP_LOGD(TAG, "Captured %ux%u JPEG (%u bytes)", width, height, (unsigned) this->jpeg_size_);
}

// ---------------------------------------------------------------------------
// handle_screenshot_()  –  runs in esp_http_server's task, NOT the main loop
// ---------------------------------------------------------------------------
esp_err_t LvglScreenshot::handle_screenshot_(httpd_req_t *req) {
  LvglScreenshot *self = instance_;
  if (!self || !self->jpeg_buf_) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Component not ready");
    return ESP_FAIL;
  }

  // Only one capture at a time.
  if (self->in_progress_) {
    httpd_resp_set_status(req, "503 Service Unavailable");
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "Capture in progress, try again");
    return ESP_OK;
  }
  self->in_progress_ = true;

  // Ask the main loop to do the capture.
  xSemaphoreGive(self->capture_requested_);

  // Wait up to 3 s for the main loop to finish (it runs at ~60 Hz so ~16 ms typical).
  if (xSemaphoreTake(self->capture_done_, pdMS_TO_TICKS(3000)) != pdTRUE) {
    self->in_progress_ = false;
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Capture timed out");
    return ESP_FAIL;
  }

  if (self->jpeg_size_ == 0) {
    self->in_progress_ = false;
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Snapshot failed");
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=\"screenshot.jpg\"");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store");

  // Stream the JPEG in 4 KB chunks.
  const size_t CHUNK = 4096;
  size_t sent = 0;
  esp_err_t ret = ESP_OK;
  while (sent < self->jpeg_size_) {
    size_t chunk_len = std::min(CHUNK, self->jpeg_size_ - sent);
    ret = httpd_resp_send_chunk(req, (const char *) self->jpeg_buf_ + sent, (ssize_t) chunk_len);
    if (ret != ESP_OK) {
      ESP_LOGW(TAG, "Failed to send chunk at offset %u", (unsigned) sent);
      break;
    }
    sent += chunk_len;
  }

  // Terminate chunked transfer.
  httpd_resp_send_chunk(req, nullptr, 0);

  self->in_progress_ = false;
  return ret;
}

}  // namespace lvgl_screenshot
}  // namespace esphome

#endif  // USE_ESP_IDF
