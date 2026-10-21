#pragma once

#include <atomic>
#include <cstdint>
#include <string>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esphome/core/component.h"

// IDF declares sdmmc_card_t as a typedef of an unnamed struct, so it can't be forward-declared.
#include "sd_protocol_types.h"

namespace esphome {
namespace cctv_sdcard {

/// Mounts a microSD card over SDMMC (1- or 4-bit) and keeps room for recordings by deleting the
/// oldest *.avi files. Mounting, space checks and deletions run in a background task.
class CctvSdCard : public Component {
 public:
  void set_pins(int clk, int cmd, int d0, int d1, int d2, int d3) {
    this->clk_ = clk;
    this->cmd_ = cmd;
    this->d_[0] = d0;
    this->d_[1] = d1;
    this->d_[2] = d2;
    this->d_[3] = d3;
  }
  void set_bus_width(int width) { this->width_ = width; }
  void set_power_pin(int pin, bool active_low) {
    this->power_pin_ = pin;
    this->power_active_low_ = active_low;
  }
  void set_high_speed(bool high) { this->high_speed_ = high; }
  void set_mount_point(const std::string &path) { this->mount_point_ = path; }
  void set_recordings_dir(const std::string &path) { this->recordings_dir_ = path; }
  void set_min_free_mb(uint32_t mb) { this->min_free_mb_ = mb; }
  void set_keep_newest(int n) { this->keep_newest_ = n; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  bool is_mounted() const { return this->mounted_; }
  float get_total_gb() const { return this->total_mb_ / 1024.0f; }
  float get_free_gb() const { return this->free_mb_ / 1024.0f; }
  uint32_t get_free_mb() const { return this->free_mb_; }
  uint32_t get_recording_count() const { return this->file_count_; }
  uint32_t get_deleted_count() const { return this->deleted_; }
  const std::string &get_recordings_dir() const { return this->recordings_dir_; }
  /// Ask the background task to re-check now (e.g. after inserting a card).
  void refresh() { this->refresh_requested_ = true; }

 protected:
  static void task_(void *arg);
  void run_();
  bool mount_();
  void unmount_();
  bool update_space_();
  /// Counts recordings and, if `remove_oldest`, deletes the oldest (keeping keep_newest).
  /// Returns true if one was deleted.
  bool scan_(bool remove_oldest);

  int clk_{11}, cmd_{14};
  int d_[4]{9, 13, 42, 12};
  int width_{4};
  int power_pin_{43};
  bool power_active_low_{true};
  bool high_speed_{false};
  std::string mount_point_{"/sdcard"};
  std::string recordings_dir_{"/sdcard/cctv"};
  uint32_t min_free_mb_{1024};
  int keep_newest_{8};

  sdmmc_card_t *card_{nullptr};
  TaskHandle_t task_handle_{nullptr};
  std::atomic<bool> mounted_{false};
  std::atomic<bool> refresh_requested_{false};
  std::atomic<uint32_t> total_mb_{0};
  std::atomic<uint32_t> free_mb_{0};
  std::atomic<uint32_t> file_count_{0};
  std::atomic<uint32_t> deleted_{0};
  bool warned_full_{false};
};

}  // namespace cctv_sdcard
}  // namespace esphome
