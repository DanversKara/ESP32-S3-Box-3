Overview

This project is based on the work by

https://github.com/chrisdunnname/esphome-s3-box-3-lvgl.  

It has been modified to better integrate with my devices and Home
Assistant setup.

---
Non Secure vs Secure Setup

1. Secure = Front Door Button has 2 Options, Option A will require a Pin Code to press unlock front door, while Option B you can uncheck needing a pin under settings that wont ask for a pin encase you wanna leave this device near the front door or at desk to unlock/lock fast while Option A is encase you wanna mount this touch screen outside and secure screen with pass code i mean a pin code.

2. Secure Settings can only be accssed thru typing in a 2nd pass code that isnt the same as the front door code, inside secure settings is the same settings from Non Secure just behind a hidden settings menu.

3. If the device is away from wifi the screen switches to NO CONNECTION making the device a brick and it cant be used for any period not even a clock making the a brick and them throw it away unless they know how to re-program it.

---

Changes

Removed the bottom navigation bar (Left / Home / Settings / Right) (the red circle on unit, is the home button)

Navigation is now handled entirely by swipe gestures\

Provides increased usable screen space

Redesigned UI buttons for improved clarity and consistency

Added support for box sensors

Integrated CCTV functionality using IP Webcam (Android)

Added a screensaver shortcut

Redesigned the screensaver for a cleaner, more modern appearance

Added a dedicated timer icon to reduce reliance on voice/AI commands

Replaced the battery icon with a percentage display in the top bar

(Home and Screensaver)

Removed the analog clock

Reduces memory usage and code size\

Aligns with a preference for digital display

Removed external GitHub-hosted images

Reduces resource usage\

Improves performance and load consistency\

i replaced HAL, Default and all other wake words with OKAY Computer

also turn off notificaiton sound, turn off wake sound, i find it delays ai from waking up to wake names. make sure you tunr on enable speaker.

Replaced with lightweight icons and cleaner text

*Swipe was already there and alarm clock was there by tapping the time top left.

ESPHome + Home Assistant Setup — Important
⚠️ Important: Home Assistant Setup After Flashing - How to connect ESP to HA Video is located here https://github.com/DanversKara/ESP32-S3-Box-3/tree/main/How_To_Connect_ESP_TO_Home_Assistant encase you dont know how or where to start.

After flashing new firmware, you must complete the following steps in Home Assistant. If you skip these steps, the ESP device may not be able to communicate with Home Assistant. Before assuming there is a problem with the firmware or code, verify the integration is configured correctly.

Setup Steps

Flash the new firmware to your ESP device.

In Home Assistant, go to:
Settings → Devices & Services

Select Add Integration and search for ESPHome.

Select ESPHome, then click Add.

Enter the device's API Key when prompted.

Click the ⚙️ Gear icon for the ESPHome device.

Make sure the required permissions/subscriptions are allowed.

Configure your Bluetooth options and select the appropriate options for your setup.

Click Submit.

Next, open the ⋮ (three-dot) menu next to the device and select Reconfigure.

Verify the IP address/hostname and port are correct, then submit the configuration.

Finally, test the device and verify that all of its features are working correctly.

🚨 If You Skip These Steps

If ESPHome cannot communicate with Home Assistant after flashing, do not immediately assume the firmware/code is broken.

First, verify that:

The ESPHome integration is installed correctly.
The correct API Key was entered.
The device has the required permissions/subscriptions enabled.
Your Bluetooth options are configured correctly.
The IP/hostname is correct.
The ESPHome API port is correct.
The device has successfully been reconfigured in Home Assistant.

A fresh firmware flash does not automatically guarantee that Home Assistant is correctly configured to communicate with the device.
