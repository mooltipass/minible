## [](#header-2)Guide to updating your Mini BLE  
   
### [](#header-3)Introduction: verifying the bundle version present on your device, understanding your update file name

![](https://github.com/mooltipass/minible/blob/gh-pages/images/minible_update_guide/ble_bundle_version.png?raw=true)  
  
Updating your Mooltipass Mini BLE means upgrading its internal bundle and internal firmwares.  
To discover your device bundle version, start Moolticute and head to its **About** tab.  
In it you will discover your bundle version. **If your bundle version isn't displayed, your version is 0**.  
  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/minible_update_guide/update_file_win.png?raw=true)  
  
Your update file should be in the following format: update\_**xxxx**\_from_bundle\_**a**\_to\_**b**\_**yyyyyyyyyyy**.img, where:  
- **xxxx** is your device serial number
- **a** is the bundle version currently present on your device
- **b** is the new bundle version about to be flashed to your device
- **yyyyyyyyyyy** is the upload password to start the flashing procedure
  
### [](#header-3)Update Step 0: Install Moolticute
If not already installed, please visit [our website](https://www.themooltipass.com/setup/) to download the latest Moolticute version.
  
  
### [](#header-3)Update Step 1: Enable update mode on your Mini BLE
Connect your Mooltipass Mini BLE to your computer **through USB** and make sure your device is fully charged:  
  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/rf_debug_guide/welcome_screen.png?raw=true)  
  
Insert a card **the wrong way around** (chip facing up) to get to this screen:  
  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/rf_debug_guide/invalid.png?raw=true)  
  
    
### [](#header-3)Update Step 2: Enable Moolticute's developer tab and upload your file

In Moolticute, press CTRL-SHIFT-F3 (Shift+Fn+Cmd+F3 on Mac) in such a way that **all** keys are pressed at the end of the shortcut.  
The BLE Dev tab will appear in the top part of the application.  
  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/minible_update_guide/minible_update_with_password.png?raw=true)  

In the "Upload Password" text field, enter the hexadecimal string present in your update file name (the 'yyyyyyyyyyy' in update\_xxxx\_from_bundle\_a\_to\_b\_**yyyyyyyyyyy**.img).  
Then click on the "Select Bundle File" button and select your update file. The upload will start automatically and your will display this screen:    
  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/minible_update_guide/firmware_file_update.png?raw=true)  
   
At the end of the upload, your device will reboot and update itself. **Do not disconnect your device during the update process**.  
If your device reboots and displays "No aux MCU", simply disconnect and reconnect your USB cable.   
