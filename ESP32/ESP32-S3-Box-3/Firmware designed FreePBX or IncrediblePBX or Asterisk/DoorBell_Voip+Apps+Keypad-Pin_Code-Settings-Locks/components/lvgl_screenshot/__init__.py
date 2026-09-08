"""LVGL Screenshot Component.

Serves an on-demand JPEG snapshot of whatever's currently on the LVGL screen
(any page - home, settings, dialpad, in-call, etc.) over a small HTTP server
running on the device itself. Nothing is captured or encoded until a request
actually comes in, so there is no ongoing CPU/RAM cost while idle.

Usage in YAML:

    external_components:
      - source:
          type: local
          path: components
        components: [lvgl_screenshot]

    lvgl_screenshot:
      port: 8080        # optional, default 8080
      jpeg_quality: 80   # optional, default 80 (1-100)

Then browse to http://<device-ip>:8080/screenshot for a fresh JPEG each time.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PORT

CODEOWNERS = ["@user"]
DEPENDENCIES = ["lvgl"]
AUTO_LOAD = ["lvgl"]

CONF_JPEG_QUALITY = "jpeg_quality"

lvgl_screenshot_ns = cg.esphome_ns.namespace("lvgl_screenshot")
LvglScreenshot = lvgl_screenshot_ns.class_("LvglScreenshot", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LvglScreenshot),
        cv.Optional(CONF_PORT, default=8080): cv.port,
        cv.Optional(CONF_JPEG_QUALITY, default=80): cv.int_range(min=10, max=100),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_port(config[CONF_PORT]))
    cg.add(var.set_quality(config[CONF_JPEG_QUALITY]))

    # LVGL's snapshot API (lv_snapshot_take) is compiled out of ESPHome's LVGL
    # build by default. ESPHome only manages a fixed allow-list of LV_USE_*
    # defines in its generated lv_conf.h and force-disables anything outside
    # that list, so this has to be turned on via a raw compiler define instead
    # (which lv_conf_internal.h's #ifndef guard picks up ahead of its own
    # default-off fallback).
    cg.add_build_flag("-DLV_USE_SNAPSHOT=1")
