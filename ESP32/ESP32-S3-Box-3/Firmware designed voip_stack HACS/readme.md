FYI: prob wont be any more updates to this project as i dont think the choppy speaker sound can be fixed on this device, i might have to try another ESP and move on from ESP32-S3-Box-3 and upgrade to something better  *if you want voip on ESP32-S3-BOX3 please use this code/firmware at https://github.com/DanversKara/ESP32-S3-Box-3/tree/main/ESP32/ESP32-S3-Box-3/Firmware%20designed%20FreePBX%20or%20IncrediblePBX%20or%20Asterisk

* BETA TESTING RIGHT NOW - USE AT YOUR OWN RISK - CODE IS TESTED TO WORK ON ESP32-S3-Box-3 Long as you setup voip_stack thru HACS and intergration voip_stack and make a browser account by clicking add phone, after flashing and dialing that number from ESP32-S3-Box-3 it should accept the call and if you dial ext 101 from browser it should call the ESP32-S3-Box-3. 

This very is crackly audio, basic theme, Apps Page, Basic Settings, Basic Volume Control.
1. install HACS voice_stack
2. integration voice_stack
3. goto voice_stacks and add 1 browser by clicking add phone
4. goto dashboard and add by card bottom of scroll page look for by card called voice_card or search for it
5. add to dashboard and select your browser card and ext number you set.
6. go back to devices and services goto voice_stack and click on 3 dot menu top of voice stack and look for Reconfigure and set the following
- HA port example 192.168.8.162 do not add :8123, then toggle switch on everything but Enable Assist intents, Include voice assistant and Enable optional sip trunk (sip truck is to connect to asterisk/freepbx to place real calls to the real world if you know how), toggle on everything else, then click submit.
7. flash the eps and see if you can place a call to the ext number of the browser card you made.

Notice: home assistant must support HTTPS not HTTP you can also use a HTTPS Domain if you know how, you cant use HTTP and must have a MIC Audio on the devices you try or the calls wont connect.
