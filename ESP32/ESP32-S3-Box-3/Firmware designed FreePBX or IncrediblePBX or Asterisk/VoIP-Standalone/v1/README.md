# ESP32-S3-Box-3 — Smart Intercom / Speed Dial Panel

An [ESPHome](https://esphome.io/) configuration that turns an **M5Stack / Espressif ESP32-S3-Box-3** into a wall-mounted VOIP intercom and Home Assistant control panel, with:

- 📞 **SIP/VOIP calling** (speed dial + manual dialpad) via [`sip_client`](https://github.com/eigger/espcomponents)
- 💬 **Two-way text messaging** with other SIP extensions (e.g. Zoiper) — MSGS button in the top bar, with an unread badge, the 5 newest texts, and a popup + chime on the home screen when a text arrives
- 🔔 Ringtone playback on incoming calls (RTTTL), with a "Test Ringtone" diagnostic button
- 💡 LCD light control (brightness) synced live with Home Assistant
- ⚙️ On-device settings: volume, backlight, mute, auto-answer, keypad/dialpad
- 🖥️ A full [LVGL](https://esphome.io/components/lvgl) touchscreen UI (home screen, in-call screen, LCD, settings, keypad/dialpad)
- 🔋 Battery voltage/percentage reporting, Wi-Fi/HA connection status icons, live clock
- 💡 Home Assistant doesn't need to be Online/Active for VOIP CALLS to work, the server can be power off/offline and still place and accept calls since its coming from FreePBX / IncrediblePBX which you need to install and host yourself its not included in this as well as your own voip provider. For Local Calls/Intercom no voip provider is needed. if you have Home Assistant APPS you should still be able to install asterisk and get local/intercom calls working that away (which will need HA to be online/active/power on since HA is hosting it once you install and set it up) and program your own voip provider if you know how to do all that, i use FreePBX / IncrediblePBX cause i know that.
- <b>❌ MIC MUTE / MICROPHONE MUTE BUTTON DOSE NOT WORK YET, you cant mute calls, if you mute for any reason, not in call, in a call, etc, you have to reboot for callers to hear you.</b>

---
YOU MUST LOOK AT FOLDER IMAGES then look at photo FreePBX_EXT_Settings_for_voip_stack.png and set your EXT for PBX to connect to voip_stack as shown in photo, just 1 settings not matching the photo you risk no connection. make sure for sure that you turn on Media Encryption and select DTLS-SRTP (not recommended) then scroll down and look for Enable DTLS to YES. if you dont no connection to HA.
---

## Hardware

| Component | Details |
|---|---|
| Board | ESP32-S3-Box-3 (`esp32s3box`), 16MB flash, 240MHz, octal PSRAM |
| Mic | ES7210 ADC over I2S |
| Speaker/DAC | ES8311 over I2S |
| Display / touch | Built-in LVGL touchscreen (SPI) |
| LED | Backlight, PWM via `GPIO47` |
| Battery | Voltage divider on `GPIO10` |

The mic and speaker share the same physical I2S pins (`GPIO45` LRCLK, `GPIO17` BCLK, `GPIO2` MCLK). One side must act as I2S clock **primary** (master) and the other as **secondary** (slave) — see [Known Issues](#known-issues--fixes) below.

---

## Prerequisites

- [ESPHome](https://esphome.io/) 2024.8.0 or newer
- A running [Home Assistant](https://www.home-assistant.io/) instance
- A SIP/VOIP server reachable on your network (e.g. Asterisk, FreePBX, or a similar PBX) with at least one extension for this device
- For texting: your own-pbx install with the messages patch applied (see [5. Text messaging](#5-text-messaging-messages-app)) — the panel's `sip_client` is voice-calls only and can't do SIP MESSAGE itself, so texts go panel → Home Assistant → own-pbx → SIP MESSAGE
- The external component [`eigger/espcomponents`](https://github.com/eigger/espcomponents) (pulled in automatically via `external_components:`)

---

## Setup

### 1. Secrets

Create a `secrets.yaml` alongside this file with:

```yaml
wifi_ssid: "YourWiFiName"
wifi_password: "YourWiFiPassword"
api_key: "base64-encoded-32-byte-key"        # generate with `esphome secrets` or `openssl rand -base64 32`
voip_test_account_extension: "705"            # your SIP extension/username
voip_test_account_password: "yourSipPassword"
settings_pin: "1234"                          # PIN to enter the on-device Settings page
door_pin: "5678"                              # PIN to unlock the door from the panel
pbx_msg_token: "token"
pbx_esp_ext: "8803"
```

### 2. Substitutions

Edit the `substitutions:` block at the top of the yaml to match your setup:

```yaml
substitutions:
  name: esp32-s3box-3
  friendly_name: ESP32-S3-Box-3
  external_media_player: living_room_speaker
  home_assistant_host: http://192.168.8.162:8123
```

Also update the SIP server address under `sip_client:`:

```yaml
sip_client:
  server: "192.168.8.220"   # your SIP/PBX server IP
```

### 3. Speed dial numbers

The two speed-dial contacts are exposed as editable `text:` entities in Home Assistant (`Speed Dial Office Number/Label`, `Speed Dial Cell Number/Label`), or you can just change the `initial_value:` fields directly in the yaml before first flash.

### 4. Flash

```bash
esphome run speed_dial.yaml
```

First flash needs a USB cable; after that, OTA updates work over Wi-Fi.

### 5. Text messaging (Messages app)
- notice this text message system was devoplted for https://github.com/DanversKara/MyPersonalPBX unsure and not tested if it works for FreePBX / IncreiblePBX, youll have to feed this into Cluade or Meta MuseAi and see if it can configure this for FreePBX/IncreiblePBX and also go thru https://github.com/DanversKara/MyPersonalPBX to get the correct API details and change them based off FreePBX/IncreiblePBX APIS

The panel is already wired for it (MSGS button in the top bar) — you just need the other two ends. This uses the exact same PBX patch and Home Assistant package as the full Apps build; nothing is standalone-specific except where the button lives.

**a) own-pbx brain** — needs a small patch (`PBX_STATUS_HOST` bind + token auth, `store` mode on `/messages/send`, and a `/messages/inbox` endpoint):

```bash
cd <your own-pbx dir>   # the folder containing pbx-brain/
patch -p1 < own-pbx-brain-messages.patch
```

```ini
# e.g. /etc/systemd/system/pbx-brain.service.d/msg.conf, then
# systemctl daemon-reload && systemctl restart pbx-brain
[Service]
Environment=PBX_STATUS_HOST=0.0.0.0
Environment=PBX_MSG_TOKEN=<a long random token — save it, HA needs it>
```

Verify: `curl -H "X-Msg-Token: <token>" "http://<pbx-ip>:8099/messages/inbox?ext=<box-ext>&limit=5"` should return `{"messages": [...], "ok": true}`, and a wrong token should return `{"ok": false, "error": "bad token"}`.

**b) Home Assistant** — drop `ha_pbx_messages.yaml` into `/config/packages/` (⚠️ the filename MUST use underscores — `ha-pbx-messages.yaml` with dashes is rejected as an invalid package slug and the whole package silently never loads), and make sure `configuration.yaml` has:

```yaml
homeassistant:
  packages: !include_dir_named packages
```

Edit the token, PBX IP, and the box's extension near the top of the package file (the `inbox_line` entity names follow your ESPHome node's name — `text.<node>_inbox_line_1` etc.), then restart HA. Check Developer Tools → States: `sensor.pbx_inbox` should show a number (the newest message id).

The extension the panel texts as needs `msg_out` enabled and a messages quota in own-pbx; whoever you text needs `msg_in` enabled. Quotas/blocks are enforced server-side, same as the portal — denied sends show up as a "PBX text not sent" notification in HA with the reason.

---

## Features in detail

**Incoming calls** — `sip_client.on_incoming_call` switches the screen to the in-call page, flashes the LED, and plays the RTTTL ringtone on a loop until answered, declined, or the caller hangs up. If **Auto Answer** is enabled (toggle in Settings), the call is picked up automatically instead of ringing.

**Speed dial / dialpad** — Home screen has one-tap buttons for the two configured contacts, plus a manual dialpad screen for any other extension.

**Settings page** — Master volume (+/‑ buttons), backlight brightness slider, speaker mute switch and the ringtone test button.

---

## Known issues & fixes

### Test Ringtone / incoming call only produces a "pop", no tone

**Cause:** the mic and speaker share one physical I2S bus. Whichever side is `i2s_mode: secondary` can only stream while the `primary` side is actively clocking the bus. In earlier revisions of this config, the **microphone** was `primary`, but the mic is only started by `sip_client` during an active call — so outside of a call there's no clock running, and any speaker output (RTTTL, TTS, etc.) gets exactly one buffer write (the "pop") before stalling.

**Fix:** make the **speaker** the clock primary and the **mic** secondary, so audio playback works standalone (ringtone, test button) and the mic still works fine once a call actually starts:

```yaml
microphone:
  - platform: i2s_audio
    id: box_mic
    i2s_audio_id: mic_bus
    i2s_mode: secondary   # was: primary

speaker:
  - id: i2s_audio_speaker
    platform: i2s_audio
    i2s_audio_id: speaker_bus
    i2s_mode: primary     # was: secondary
```

This is already applied in this yaml.

---

## Repo structure

```
.
├── speed_dial.yaml     # this file — full ESPHome device config
├── secrets.yaml         # NOT committed — Wi-Fi, API key, SIP creds, PINs
└── README.md
```

> ⚠️ **Never commit `secrets.yaml`.** Add it to `.gitignore`.

---

## License

Add your preferred license here (MIT, GPL-3.0, etc.).
