## [](#header-2)Guide to updating your Mini BLE  

### [](#header-3)Step 1: Enable Update Mode on your Mini BLE
Connect your Mooltipass Mini BLE to your computer **through USB** and make sure your device is fully charged:  
  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/rf_debug_guide/welcome_screen.png?raw=true)  
  
Insert a card **the wrong way around** to get to this screen:  
  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/rf_debug_guide/invalid.png?raw=true)  
  
    
### [](#header-3)Step 2: If Not Done, Install Moolticute  
If using Windows or Linux, install the latest moolticute beta [here](https://betas.themooltipass.com)  
If using Ubuntu, install from [here](https://launchpad.net/~mooltipass/+archive/ubuntu/moolticute-beta)  
If using MacOS, install from [here](https://github.com/mooltipass/moolticute/releases)  
  
    
### [](#header-3)Step 3: Identify your Mini BLE Firmware Version

![](https://github.com/mooltipass/minible/blob/gh-pages/images/minible_update_guide/ble_about_tab_fw_version.png?raw=true)  

On top of Moolticute, click on the About tab.   
If your main MCU version is v0.54 or above, [follow the instructions for Moolticute v0.44.22](#step-4-moolticute-versions-v04422-and-above)  
If your main MCU version is below v0.54, [downgrade moolticute to v0.44.19](https://betas.themooltipass.com/v0.44.19-testing/), [follow the instructions for Moolticute below v0.44.22](#step-4-moolticute-versions-below-v04422) and finally update Moolticute to the latest version.  
  
    
### [](#header-3)Step 4: Moolticute versions v0.44.22 and above

In Moolticute, press CTRL-SHIFT-F3 (Shift+Fn+Cmd+F3 on Mac) in such a way that all keys are pressed at the end of the shortcut. The BLE Dev tab will appear on the top part of the application.  
  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/minible_update_guide/ble_dev_tab_new_upload.png?raw=true)  
  
Click on the "Select Bundle File" button, select the bundle file we sent you by email.  
The upload will start automatically:  
  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/minible_update_guide/firmware_file_update.png?raw=true)  
   
At the end of the upload, your device will reboot and update itself.  
**Do not disconnect your device during the update process**.  
If the device reboots and displays "No aux MCU", simply disconnect and reconnect your USB cable.  
  
  
### [](#header-3)Step 4: Moolticute versions below v0.44.22

In Moolticute, press CTRL-SHIFT-F3 (SHIFT-Command-F3 on Mac) in such a way that all 3 keys are pressed at the end of the shortcut.  
The BLE Dev tab will appear on the top part of the application. 
  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/minible_update_guide/ble_dev_tab_upload_flash.png?raw=true)  
  
Click on the "Select Bundle File" button, select the bundle file we sent you by email.  
The upload will start automatically:  
  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/minible_update_guide/firmware_file_update.png?raw=true)  
   
At the end of the upload, click on **Update platform**.  
The platform will then reboot and update itself.  
**Do not disconnect your device during the update process**.  
