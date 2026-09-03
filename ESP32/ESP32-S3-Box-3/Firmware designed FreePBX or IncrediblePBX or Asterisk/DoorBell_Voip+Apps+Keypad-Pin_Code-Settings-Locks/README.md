# CallGate — ESP32-S3-Box-3 SIP Doorbell / Intercom

Custom ESPHome firmware that turns an **M5Stack / Espressif ESP32-S3-Box-3** into a
SIP-based video-free doorbell and intercom, fully integrated with Home Assistant.

Tap the screen → it rings a SIP extension inside your house. Answer calls, dial
extensions manually, unlock the front door, and control audio — all from a
touchscreen mounted at your gate or front door.

---

## Features

- **SIP doorbell button** — a full-screen "PRESS HERE" surface plus a pulsing
  red bell icon rings a configurable extension (`doorbell_extension`) over SIP.
- **Full VoIP dial pad** — place calls to any extension, not just the doorbell one.
- **Incoming call handling** — ringtone playback, optional auto-answer, live
  call timer, in-call volume and mic gain controls.
- **Home Assistant native API integration** — door unlock via an HA script,
  LCD backlight exposed as a light entity, battery voltage/percent reported
  as diagnostic sensors, Bluetooth proxy.
- **PIN-protected settings menu** — unlock door, dial pad, audio/mic control,
  LCD brightness, and a "homepage exit" shortcut.
- **Screen rotation** — 0° / 90° / 180° / 270°, selectable from Home Assistant
  (`Screen Rotation` select entity), with every page's layout re-flowing
  automatically for portrait vs. landscape.
- **After-hours availability banner** — a home-page banner (with custom
  message text and color) that automatically switches based on HA-editable
  "Available From" / "Available Until" times, including windows that wrap
  past midnight.
- **Connection-loss overlays**
  - **HA unreachable** — full-screen blinking red "DOOR BELL UNAVAILABLE AT
    THIS TIME" message whenever the Home Assistant API client disconnects.
  - **Wi-Fi lost** — takes priority over the HA overlay above: a full-screen
    white background with bold red **"HELP I BEEN STOLEN — RETURN TO PO BOX
    1357, LINDSAY, CA 93247"** message the instant the Wi-Fi link drops,
    correctly laid out at every screen rotation.
- **Animated boot screen** — spinner + "BOOTING..." dots shown until the
  device has actually connected to Home Assistant, so the doorbell never
  looks "ready" while it's silently offline.
- **Matrix-style animated background** on the home page for a distinctive look.

---

## Hardware

| Component | Notes |
|---|---|
| Board | ESP32-S3-Box-3 (`esp32s3box`), 16MB flash, octal PSRAM |
| Display | MIPI-SPI `S3BOX` panel via LVGL |
| Touch | GT911 capacitive touchscreen |
| Audio out | ES8311 DAC → onboard speaker |
| Audio in | ES7210 ADC → onboard mic array |
| Buttons | Side top button, physical mute switch, capacitive "Home" button |
| Sensors | Battery voltage (ADC) → battery percent |

---

## Software Stack

- [ESPHome](https://esphome.io/) (`min_version: 2024.8.0`), ESP-IDF framework
- [LVGL](https://esphome.io/components/lvgl) for the UI
- External component: [`sip_client`](https://github.com/eigger/espcomponents) for SIP/VoIP
- Local component: `lvgl_screenshot` for pulling live screenshots off the device (port `8080`)
- Home Assistant native `api:` integration

---

## Live Screenshot Mirror (watch the screen from Home Assistant)

The `lvgl_screenshot` local component serves a live JPEG of whatever's on the
device's screen at `http://<esp-ip>:8080/screenshot`. Pull that into Home
Assistant as a camera entity and you can watch what a visitor is looking at
in real time — and confirm a reboot actually came back up — right from your
dashboard.

### 1. Install the component files (via the ESPHome add-on's File Editor)

1. In Home Assistant, go to **Settings → Add-ons**, install and open **File Editor**.
2. Confirm the **ESPHome** add-on is installed (install it if it isn't).
3. Open the **File Editor** app.
4. Open the `esphome` folder.
5. Create a `components` folder, and inside it a `lvgl_screenshot` folder.
6. Upload all four component files into `esphome/components/lvgl_screenshot/`:
   - `__init__.py`
   - `lvgl_screenshot.cpp`
   - `lvgl_screenshot.h`
   - `stb_image_write.h`

This matches the `external_components:` block in the YAML, which loads
`lvgl_screenshot` from `esphome/components/` and serves it on port `8080`.

### 2. Flash the firmware

Open the **ESPHome** dashboard and flash `callgate-v9.yaml` as usual.

### 3. Verify the screenshot endpoint

Once the device has booted, visit:

```
http://<esp-ip>:8080/screenshot
```

You should see a live JPEG of the on-device screen. If it loads, you're
good to go.

### 4. Add it to Home Assistant as a Generic Camera

1. Go to **Settings → Devices & Services**.
2. Click **+ Add Integration** (bottom right).
3. Search for **Generic Camera**.
4. Fill in the form:
   - **Still Image URL**: `http://<esp-ip>:8080/screenshot`
   - **Stream URL**: leave blank (this is a still JPEG, not MJPEG/RTSP)
   - **Verify SSL**: off (plain HTTP on your LAN)
   - **Name**: something like `S3-Box Screenshot`
5. HA will preview the image live in the setup dialog — if you see the
   doorbell UI, click **Submit**.

Equivalent YAML, if you'd rather define it directly instead of through the UI:

```yaml
camera:
  - platform: generic
    name: "S3-Box Screenshot"
    still_image_url: "http://192.168.8.217:8080/screenshot"
    limit_refetch_to_url_change: false
    framerate: 1
```

### 5. Add it to your dashboard

Add the new camera entity to a Lovelace dashboard and set its card to
**Live**. Now you can watch what someone at the door is looking at in real
time, and confirm the screen came back correctly after a reboot — without
walking outside.

---

## Required Secrets

Create a `secrets.yaml` alongside this file with:

```yaml
wifi_ssid: "your-wifi-ssid"
wifi_password: "your-wifi-password"
api_key: "base64-encoded-32-byte-key"     # esphome generates this for you
settings_pin: "1234"                       # PIN to enter the on-device Settings menu
voip_test_account_extension: "600"         # SIP extension this device registers as
voip_test_account_password: "your-sip-password"
```

---

## Key Substitutions

Edit these at the top of the YAML to match your setup:

| Substitution | Purpose |
|---|---|
| `home_assistant_host` | URL of your Home Assistant instance |
| `doorbell_extension` | SIP extension rung when the doorbell is pressed |
| `door_lock_script_entity` | HA script entity toggled by "Unlock Door" |
| `lcd_backlight_entity` | HA light entity mirroring the LCD backlight |
| `wifi_fallback_pwd` | Password for the device's fallback AP if Wi-Fi fails |

SIP server address (`192.168.8.220` by default) is set directly under the
`sip_client:` block.

---

## Home Assistant Entities Exposed

- **Light** — `LCD Backlight`
- **Sensors** — `Battery Voltage`, `Battery Level` (diagnostic)
- **Switches** — `Speaker Mute`, `Intercom Auto Answer`, `speaker_enable`
- **Select** — `Screen Rotation` (0° / 90° / 180° / 270°)
- **Text** — `Message - Available`, `Message - After Hours`
- **Datetime** — `Available From`, `Available Until`
- **Button** — `Test Ringtone` (diagnostic)
- **Binary sensor** — `Mic Mute Switch` (diagnostic)

---

## On-Device Screens

1. **Boot** — animated spinner, shown until HA connects
2. **Home** — doorbell press surface, availability banner, connection overlays
3. **Settings PIN** — numeric keypad gate for the settings menu
4. **Settings** — unlock door, dial pad, audio/mic control, LCD brightness, exit
5. **Dial Pad** — full 3×4 keypad with call/end/home actions
6. **In-Call** — volume/mic controls, call timer, answer/hangup
7. **Audio / Mic Control** — auto-answer toggle, speaker mute, volume, mic gain
8. **LCD Brightness** — backlight slider

---

## Flashing

```bash
esphome run callgate-v9.yaml
```

Or compile only:

```bash
esphome compile callgate-v9.yaml
```

---

## Notes / Gotchas

- Wi-Fi loss also drops SIP registration and the HA API connection — by
  design, the "stolen" overlay takes priority over the HA-unavailable
  overlay whenever Wi-Fi itself is down, since Wi-Fi loss implies HA loss too.
- The mic and speaker share an I2S bus; **never** stop/restart the mic
  capture independently, or the speaker will desync until reboot (see
  inline comments on the `hw_mute_switch` binary sensor).
- `sip_client` only fires `on_registered` once per boot — there's no
  "unregistered" callback — so live SIP status is inferred from
  `wifi_connected && sip_registered` rather than a single flag.
