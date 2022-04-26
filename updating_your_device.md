## [](#header-2)Guide to updating your Mini BLE  
   
### [](#header-3)Introduction
  
Before updating your device, **make sure to backup your user profile** by following the instructions present in our [user manual](https://github.com/mooltipass/minible/raw/master/MooltipassMiniBLEUserManual.pdf), section III. F).  
  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/minible_update_guide/ble_bundle_version.png?raw=true)  
  
Updating your Mooltipass Mini BLE means upgrading its internal bundle and internal firmwares.  
To discover your device bundle version, start Moolticute and head to its **About** tab.  
In it you will discover your bundle version. **If it isn't displayed, your version is 0**.  
  
### [](#header-3)Update Step 0: Install Moolticute
If not already installed, please visit [our website](https://www.themooltipass.com/setup/) to download the latest Moolticute version.
  
  
### [](#header-3)Update Step 1: Enable Update Mode on your Mini BLE
Connect your Mooltipass Mini BLE to your computer **through USB** and make sure your device is fully charged:  
  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/rf_debug_guide/welcome_screen.png?raw=true)  
  
Insert a card **the wrong way around** (chip facing up) to get to this screen:  
  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/rf_debug_guide/invalid.png?raw=true)  
  
    
### [](#header-3)Update Step 2: Enable Moolticute's Developer Tab, Upload your File

In Moolticute, press CTRL-SHIFT-F3 (Shift+F3 on Mac) in such a way that **all** keys are pressed at the end of the shortcut.  
The BLE Dev tab will appear in the top part of the application.  
  
![](https://github.com/mooltipass/minible/blob/master/wiki/images/minible_update_guide/ble_dev_tab_upload_flash_new.png?raw=true)  

Click on the "Select Bundle File" button and select your update file. The upload will start automatically and your unit will display this screen:    
  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/minible_update_guide/firmware_file_update.png?raw=true)  
   
At the end of the upload, your device will reboot and update itself. **Do not disconnect your device during the update process**.  
If your device reboots and displays "No aux MCU", simply disconnect and reconnect your USB cable.   
