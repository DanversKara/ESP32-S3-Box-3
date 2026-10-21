#include "cctv_sdcard.h"

#include <cerrno>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <strings.h>
#include <sys/stat.h>

#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#include "esphome/core/log.h"

namespace esphome {
namespace cctv_sdcard {

static const char *const TAG = "cctv_sdcard";
static const uint32_t CHECK_INTERVAL_MS = 30000;

void CctvSdCard::setup() {
  if (this->power_pin_ >= 0) {
    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << this->power_pin_;
    io.mode = GPIO_MODE_OUTPUT;
    gpio_config(&io);
    gpio_set_level(static_cast<gpio_num_t>(this->power_pin_), this->power_active_low_ ? 0 : 1);  // card power on
  }
  // Low priority, next to the network tasks on core 0; 6 KB stack for FATFS + directory scans.
  if (xTaskCreatePinnedToCore(task_, "cctv_sd", 6144, this, 1, &this->task_handle_, 0) != pdPASS) {
    ESP_LOGE(TAG, "Cannot start the SD card task");
    this->mark_failed();
  }
}

void CctvSdCard::dump_config() {
  ESP_LOGCONFIG(TAG, "CCTV SD card:");
  ESP_LOGCONFIG(TAG, "  Bus: SDMMC %d-bit, %s, CLK %d CMD %d D0 %d%s", this->width_,
                this->high_speed_ ? "40 MHz" : "20 MHz", this->clk_, this->cmd_, this->d_[0],
                this->width_ == 4 ? " D1-D3 set" : "");
  ESP_LOGCONFIG(TAG, "  Power pin: %d (%s)", this->power_pin_, this->power_active_low_ ? "active low" : "active high");
  ESP_LOGCONFIG(TAG, "  Recordings: %s, keep >= %u MB free (newest %d always kept)", this->recordings_dir_.c_str(),
                (unsigned) this->min_free_mb_, this->keep_newest_);
  ESP_LOGCONFIG(TAG, "  Mounted: %s", this->mounted_ ? "yes" : "no (will retry)");
}

void CctvSdCard::task_(void *arg) { static_cast<CctvSdCard *>(arg)->run_(); }

void CctvSdCard::run_() {
  vTaskDelay(pdMS_TO_TICKS(300));  // let the card power up
  while (true) {
    if (!this->mounted_) {
      if (this->mount_()) {
        this->mounted_ = true;
        this->warned_full_ = false;
      }
    }
    if (this->mounted_) {
      if (!this->update_space_()) {
        ESP_LOGW(TAG, "SD card stopped responding (removed?); unmounting");
        this->unmount_();
      } else {
        // Make room: delete oldest recordings until min_free is met (a few per pass).
        this->scan_(false);
        int deleted = 0;
        while (this->free_mb_ < this->min_free_mb_ && deleted < 20 && this->scan_(true)) {
          deleted++;
          this->update_space_();
        }
        if (this->free_mb_ < this->min_free_mb_ && deleted == 0 && !this->warned_full_) {
          this->warned_full_ = true;
          ESP_LOGW(TAG, "SD card has only %u MB free and nothing old enough to delete", (unsigned) this->free_mb_);
        } else if (this->free_mb_ >= this->min_free_mb_) {
          this->warned_full_ = false;
        }
      }
    }
    // Sleep, but wake early on refresh().
    for (uint32_t waited = 0; waited < CHECK_INTERVAL_MS && !this->refresh_requested_; waited += 250)
      vTaskDelay(pdMS_TO_TICKS(250));
    this->refresh_requested_ = false;
  }
}

bool CctvSdCard::mount_() {
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.max_freq_khz = this->high_speed_ ? SDMMC_FREQ_HIGHSPEED : SDMMC_FREQ_DEFAULT;

  sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
  slot.width = this->width_;
  slot.clk = static_cast<gpio_num_t>(this->clk_);
  slot.cmd = static_cast<gpio_num_t>(this->cmd_);
  slot.d0 = static_cast<gpio_num_t>(this->d_[0]);
  if (this->width_ == 4) {
    slot.d1 = static_cast<gpio_num_t>(this->d_[1]);
    slot.d2 = static_cast<gpio_num_t>(this->d_[2]);
    slot.d3 = static_cast<gpio_num_t>(this->d_[3]);
  }
  slot.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {};
  mount_cfg.format_if_mount_failed = false;  // never wipe a card on its own
  mount_cfg.max_files = 8;                   // 4 recordings + headroom
  mount_cfg.allocation_unit_size = 16 * 1024;

  sdmmc_card_t *card = nullptr;
  esp_err_t err = esp_vfs_fat_sdmmc_mount(this->mount_point_.c_str(), &host, &slot, &mount_cfg, &card);
  if (err != ESP_OK) {
    static uint32_t fails = 0;
    if (fails++ % 10 == 0) {  // don't flood the log while no card is inserted
      ESP_LOGW(TAG, "No usable SD card (%s). Insert a FAT32-formatted microSD card; retrying every %u s",
               esp_err_to_name(err), (unsigned) (CHECK_INTERVAL_MS / 1000));
    }
    return false;
  }
  this->card_ = card;
  if (mkdir(this->recordings_dir_.c_str(), 0777) != 0 && errno != EEXIST)
    ESP_LOGW(TAG, "Cannot create %s (errno %d)", this->recordings_dir_.c_str(), errno);
  ESP_LOGI(TAG, "SD card mounted at %s: %s, %llu MB", this->mount_point_.c_str(), card->cid.name,
           (unsigned long long) ((uint64_t) card->csd.capacity * card->csd.sector_size / (1024 * 1024)));
  return true;
}

void CctvSdCard::unmount_() {
  if (this->card_ != nullptr) {
    esp_vfs_fat_sdcard_unmount(this->mount_point_.c_str(), this->card_);
    this->card_ = nullptr;
  }
  this->mounted_ = false;
  this->total_mb_ = 0;
  this->free_mb_ = 0;
}

bool CctvSdCard::update_space_() {
  if (this->card_ == nullptr || sdmmc_get_status(this->card_) != ESP_OK)
    return false;
  uint64_t total = 0, free_bytes = 0;
  if (esp_vfs_fat_info(this->mount_point_.c_str(), &total, &free_bytes) != ESP_OK)
    return false;
  this->total_mb_ = static_cast<uint32_t>(total / (1024 * 1024));
  this->free_mb_ = static_cast<uint32_t>(free_bytes / (1024 * 1024));
  return true;
}

static bool is_recording_name(const char *name) {
  const size_t n = strlen(name);
  return n > 4 && strcasecmp(name + n - 4, ".avi") == 0;
}

/// Sort key: "nodate_*" files (recorded before the clock was set) count as the oldest.
static bool older(const char *a, const char *b) {
  const bool a_nodate = strncmp(a, "nodate_", 7) == 0, b_nodate = strncmp(b, "nodate_", 7) == 0;
  if (a_nodate != b_nodate)
    return a_nodate;
  return strcmp(a, b) < 0;
}

bool CctvSdCard::scan_(bool remove_oldest) {
  DIR *dir = opendir(this->recordings_dir_.c_str());
  if (dir == nullptr)
    return false;
  // One pass, remembering only the oldest name, so thousands of files don't use up RAM.
  char oldest[256] = "";
  uint32_t count = 0;
  struct dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    if (entry->d_type == DT_DIR || !is_recording_name(entry->d_name))
      continue;
    count++;
    if (oldest[0] == '\0' || older(entry->d_name, oldest)) {
      strncpy(oldest, entry->d_name, sizeof(oldest) - 1);
      oldest[sizeof(oldest) - 1] = '\0';
    }
  }
  closedir(dir);
  this->file_count_ = count;
  if (!remove_oldest || oldest[0] == '\0' || count <= static_cast<uint32_t>(this->keep_newest_))
    return false;

  const std::string path = this->recordings_dir_ + "/" + oldest;
  // Never delete something modified in the last 15 minutes (it may still be recording).
  struct stat st;
  const time_t now = ::time(nullptr);
  if (stat(path.c_str(), &st) == 0 && now > 1577836800 /* clock set */ && st.st_mtime > now - 15 * 60)
    return false;
  if (remove(path.c_str()) != 0) {
    ESP_LOGW(TAG, "Cannot delete %s (errno %d)", path.c_str(), errno);
    return false;
  }
  this->deleted_++;
  this->file_count_ = count - 1;
  ESP_LOGI(TAG, "Deleted oldest recording %s to keep %u MB free", oldest, (unsigned) this->min_free_mb_);
  return true;
}

}  // namespace cctv_sdcard
}  // namespace esphome
