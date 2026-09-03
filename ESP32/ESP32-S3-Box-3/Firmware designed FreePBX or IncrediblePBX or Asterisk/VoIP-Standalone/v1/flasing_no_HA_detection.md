ESPHome + Home Assistant Setup — Important
⚠️ Important: Home Assistant Setup After Flashing - How to connect ESP to HA Video is located here https://github.com/DanversKara/ESP32-S3-Box-3/tree/main/How_To_Connect_ESP_TO_Home_Assistant encase you dont know how or where to start.

After flashing new firmware, you must complete the following steps in Home Assistant. If you skip these steps, the ESP device may not be able to communicate with Home Assistant. Before assuming there is a problem with the firmware or code, verify the integration is configured correctly.

Setup Steps

Flash the new firmware to your ESP device.

In Home Assistant, go to:
Settings → Devices \& Services

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

