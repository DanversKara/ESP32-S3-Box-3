# v5.1-messages-cctv — v4 + 128 KB internal-RAM reserve + motionEye at .238

Two changes on top of v4:
1. `CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL` 65536 -> 131072 (line 202) -
   fixes the internal-DMA-RAM exhaustion behind the err-101 SD/display
   failures and the two SIP out-of-memory crashes on 2026-10-21.
2. Camera URLs point at motionEye on **192.168.8.238** (the .212 in the
   original upload is stale - flashing it left the video black). **The floating window is gone completely** — not
hidden, removed. Everything else (VoIP, messages, lights, settings…) is
byte-for-byte untouched.

## What changed

**Removed (all floating-window traces)**
- The floating window on the home page (drag/resize/tap logic and all)
- The amber camera toggle button on the home page
- `cctv_float_apply` script, all `cctv_float_*` / position / drag globals
- The CCTV page FLOAT button
- HA: "CCTV Floating Window" switch and "CCTV Floating Camera" select
- Home page now parks all cameras on load (they idle-disconnect)

**CCTV page — now the v6 viewer**
- Full-screen **320x240** video (was 320x200 under a top button row)
- Bottom button bar is **hidden**: tap the picture to show it, auto-hides
  after 5 s. Buttons: **1 2 3 4 · FIT/STR/CROP · GRID · EXIT**
  (EXIT still goes back to Apps, as before)
- Top row: camera name (left), clock (centre), **WiFi + Home Assistant
  icons** (right, green = connected, red = not)
- FIT/STR/CROP per screen, saved across reboots: FIT keeps the shape with
  black bars, STR stretches to fill exactly, CROP fills and centre-crops

**Grid page**
- Same 2x2 grid, plus a **FIT/STR/CROP button** at the top centre so the
  grid's fit is set independently of the CCTV page's

**Home Assistant**
- New **"CCTV Camera"** select (replaces the floating-camera select):
  picking a camera switches the CCTV page and re-routes the stream if
  you're on it

**Recording (new in v4)**
- REC button on the CCTV page's button bar (now two rows: 1-4 / mode /
  GRID on top, REC / EXIT below), REC badges on the CCTV page and every
  grid cell, camera numbers turn red while recording
- **The physical red-circle button under the screen toggles REC** for the
  camera you're viewing while you're on the CCTV page. During a call it
  still hangs up; on every other page it still goes home, as before
- HA: "Record <camera>" switch per camera, Record All / Stop All buttons,
  SD Card Free / Size / Recordings on SD sensors, SD Card Mounted sensor
- Files: `/sdcard/cctv/<YYYYMMDD_HHMMSS>_<NAME>.avi`, Motion-JPEG AVI
  (plays in VLC), **1 fps**, 5-minute segments, oldest deleted below
  1 GB free (newest 8 always kept). Recording resumes after a reboot.
  Recording writes the raw stream frames - no extra decoding.
- Needs the BOX-3-SENSOR base (microSD slot), card formatted FAT32

**Unchanged on purpose**
- Camera URLs: still the motionEye HTTP ports (your own note in the file:
  the cameras' RTSP is H.264, which this component cannot decode)
- Decode rates: still 1 fps single / 1 fps grid, as you set them
- (v3 note, now outdated: SD recording was added in v4 - see above)

## Install

1. Back up your current device folder.
2. Copy `v5-messages-cctv.yaml` in (keep your `secrets.yaml`).
3. **Replace the whole `components/esp_rtsp_stream/` folder** with the one
   in this package, and **add `components/cctv_sdcard/`** next to it. This
   is required, not optional: the FIT/STR/CROP modes and the recording
   support live in these components. Swapping only the YAML will not
   compile.
4. Flash as usual.

Your `sip_client` component (from GitHub) is not in this package and is
not touched.
