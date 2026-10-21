"""SD card for CCTV recordings (ESP32-S3-BOX-3 + BOX-3-SENSOR microSD slot by default).

Mounts the card with FATFS at `mount_point`, keeps at least `min_free` space by deleting the
oldest recordings in `recordings_dir`, and reports total/free space. Everything runs in a
background task, so a slow or missing card never stalls the display.
"""

import esphome.codegen as cg
from esphome.components import esp32
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ["esp32"]

cctv_sdcard_ns = cg.esphome_ns.namespace("cctv_sdcard")
CctvSdCard = cctv_sdcard_ns.class_("CctvSdCard", cg.Component)

CONF_CLK_PIN = "clk_pin"
CONF_CMD_PIN = "cmd_pin"
CONF_DATA_PINS = "data_pins"
CONF_POWER_PIN = "power_pin"
CONF_POWER_ACTIVE_LOW = "power_active_low"
CONF_MOUNT_POINT = "mount_point"
CONF_RECORDINGS_DIR = "recordings_dir"
CONF_MIN_FREE = "min_free"
CONF_KEEP_NEWEST = "keep_newest"
CONF_HIGH_SPEED = "high_speed"

GPIO = cv.int_range(min=0, max=48)


def _validate(config):
    if len(config[CONF_DATA_PINS]) not in (1, 4):
        raise cv.Invalid("data_pins needs 1 pin (1-bit bus) or 4 pins (4-bit bus)")
    if not config[CONF_MOUNT_POINT].startswith("/"):
        raise cv.Invalid("mount_point must start with /")
    if not config[CONF_RECORDINGS_DIR].startswith(config[CONF_MOUNT_POINT] + "/"):
        raise cv.Invalid("recordings_dir must be inside mount_point")
    return config


def _parse_size(value):
    """'512MB', '2GB', '0' -> megabytes."""
    v = value.strip().upper().replace(" ", "")
    try:
        if v.endswith("GB"):
            return int(float(v[:-2]) * 1024)
        if v.endswith("MB"):
            return int(float(v[:-2]))
        return int(float(v))
    except ValueError as err:
        raise cv.Invalid(f"min_free must look like 512MB or 2GB, got {value!r}") from err


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CctvSdCard),
            # Defaults: ESP32-S3-BOX-3 with the BOX-3-SENSOR base (Espressif esp-box-3 BSP pins).
            cv.Optional(CONF_CLK_PIN, default=11): GPIO,
            cv.Optional(CONF_CMD_PIN, default=14): GPIO,
            cv.Optional(CONF_DATA_PINS, default=[9, 13, 42, 12]): cv.ensure_list(GPIO),
            cv.Optional(CONF_POWER_PIN, default=43): cv.Any(cv.one_of(-1, int=True), GPIO),
            cv.Optional(CONF_POWER_ACTIVE_LOW, default=True): cv.boolean,
            cv.Optional(CONF_HIGH_SPEED, default=False): cv.boolean,
            cv.Optional(CONF_MOUNT_POINT, default="/sdcard"): cv.string_strict,
            cv.Optional(CONF_RECORDINGS_DIR, default="/sdcard/cctv"): cv.string_strict,
            cv.Optional(CONF_MIN_FREE, default="1GB"): cv.All(cv.string, _parse_size),
            cv.Optional(CONF_KEEP_NEWEST, default=8): cv.int_range(min=1, max=1000),
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _validate,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_pins(config[CONF_CLK_PIN], config[CONF_CMD_PIN], *(config[CONF_DATA_PINS] + [-1] * 3)[:4]))
    cg.add(var.set_bus_width(len(config[CONF_DATA_PINS])))
    cg.add(var.set_power_pin(config[CONF_POWER_PIN], config[CONF_POWER_ACTIVE_LOW]))
    cg.add(var.set_high_speed(config[CONF_HIGH_SPEED]))
    cg.add(var.set_mount_point(config[CONF_MOUNT_POINT]))
    cg.add(var.set_recordings_dir(config[CONF_RECORDINGS_DIR].rstrip("/")))
    cg.add(var.set_min_free_mb(config[CONF_MIN_FREE]))
    cg.add(var.set_keep_newest(config[CONF_KEEP_NEWEST]))

    # ESPHome leaves FATFS and the SD drivers out of the build by default; bring them back.
    for name in ("fatfs", "sdmmc", "wear_levelling", "esp_driver_sdmmc", "esp_driver_sdspi"):
        esp32.include_builtin_idf_component(name)
    for fn in ("require_fatfs", "require_vfs_dir"):
        f = getattr(esp32, fn, None)
        if f is not None:
            f()
