Step 1: Open Settings, Apps, install and open File Editor
Step 2: Go into ESPHome if its already installed, if not install it
Step 3: goto APP FILE EDITOR
Step 4: open ESPHOME Folder
Step 5: make a new folder called components, then inside components, make a new folder called lvgl_screenshot
Step 6: upload all 4 files, __init__.py lvgl_screenshot.cpp lvgl_screenshot.h stb_image_write.h
Step 7: open ESPHOME and flash the callgate-v?.yaml firmware
Step 8: once booted test the ESP Mirror going to http://192.168.x.ESP_IP:8080/screenshot
Step 9: ok its working
Step 10: goto go Settings, Devices & Services, click on + ADD INTERGRATION
Step 11: look for Generic Camera and add the url http://192.168.x.ESP_IP:8080/screenshot under still images and click submit
Add it via the UI instead

Go to Settings → Devices & Services
Click + Add Integration (bottom right)
Search for "Generic Camera"
Fill in the form:
Still Image URL: http://192.168.8.217:8080/screenshot
Leave Stream URL blank (you don't have MJPEG/RTSP, just a still JPEG)
Verify SSL: off (it's plain HTTP on your LAN)
Name it something like S3-Box Screenshot
HA will preview the image live in the setup dialog — if you see your doorbell UI, click Submit

Step 12: goto Dashboard and add it and set to LIVE bam, now you can watch what users are looking at live and if you get an erorr and need to reboot live.

camera:
  - platform: generic
    name: "S3-Box Screenshot"
    still_image_url: "http://192.168.8.217:8080/screenshot"
    limit_refetch_to_url_change: false
    framerate: 1