- Here are 2 Version of the voip project, 1 Provides you a standalone voip box and 1 provides you a all in one box that also lets you voip + control your lights, IR, locks, etc but you will have to program that stuff.


# VoIP+Apps+Keypad-Pin_Code-Settings-Locks
- 📞 **SIP/VOIP calling** (speed dial + manual dialpad) via [`sip_client`](https://github.com/eigger/espcomponents)
- 🔔 Ringtone playback on incoming calls (RTTTL), with a "Test Ringtone" diagnostic button
- 🔒 A PIN-protected door lock control page
- 💡 Light control (brightness + color temperature) synced live with Home Assistant
- 📺 IR device toggles (TV, soundbar, fan) mirrored from Home Assistant switches
- ⚙️ On-device settings: volume, backlight, mute, auto-answer, PIN lock toggle
- 🖥️ A full [LVGL](https://esphome.io/components/lvgl) touchscreen UI (home screen, in-call screen, lights, IR menu, settings, PIN pad)
- 🔋 Battery voltage/percentage reporting, Wi-Fi/HA connection status icons, live clock
- 💡 Home Assistant doesn't need to be Online/Active for VOIP CALLS to work, the server can be power off/offline and still place and accept calls since its coming from FreePBX / IncrediblePBX which you need to install and host yourself its not included in this as well as your own voip provider. For Local Calls/Intercom no voip provider is needed. if you have Home Assistant APPS you should still be able to install asterisk and get local/intercom calls working that away (which will need HA to be online/active/power on since HA is hosting it once you install and set it up) and program your own voip provider if you know how to do all that, i use FreePBX / IncrediblePBX cause i know that.
- <b>❌ MIC MUTE / MICROPHONE MUTE BUTTON DOSE NOT WORK YET, you cant mute calls, if you mute for any reason, not in call, in a call, etc, you have to reboot for callers to hear you.</b>

# VoIP-Standalone/v1
- 📞 **SIP/VOIP calling** (speed dial + manual dialpad) via [`sip_client`](https://github.com/eigger/espcomponents)
- 🔔 Ringtone playback on incoming calls (RTTTL), with a "Test Ringtone" diagnostic button
- 💡 LCD light control (brightness) synced live with Home Assistant
- ⚙️ On-device settings: volume, backlight, mute, auto-answer, keypad/dialpad
- 🖥️ A full [LVGL](https://esphome.io/components/lvgl) touchscreen UI (home screen, in-call screen, LCD, settings, keypad/dialpad)
- 🔋 Battery voltage/percentage reporting, Wi-Fi/HA connection status icons, live clock
- 💡 Home Assistant doesn't need to be Online/Active for VOIP CALLS to work, the server can be power off/offline and still place and accept calls since its coming from FreePBX / IncrediblePBX which you need to install and host yourself its not included in this as well as your own voip provider. For Local Calls/Intercom no voip provider is needed. if you have Home Assistant APPS you should still be able to install asterisk and get local/intercom calls working that away (which will need HA to be online/active/power on since HA is hosting it once you install and set it up) and program your own voip provider if you know how to do all that, i use FreePBX / IncrediblePBX cause i know that.
- <b>❌ MIC MUTE / MICROPHONE MUTE BUTTON DOSE NOT WORK YET, you cant mute calls, if you mute for any reason, not in call, in a call, etc, you have to reboot for callers to hear you.</b>
