## [](#header-2)Guide to updating your Mini BLE  
  
### [](#header-3)Step1: Install Moolticute  
If using Windows or Linux, install the latest moolticute beta [here](https://betas.themooltipass.com)  
If using Ubuntu, install from [here](https://launchpad.net/~mooltipass/+archive/ubuntu/moolticute-beta)  
If using MacOS, install from [here](https://github.com/mooltipass/moolticute/releases)  

### [](#header-3)Step2: Upload and Flash Bunble  
Connect your device to your computer **using USB** to get to the welcome screen:  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/rf_debug_guide/welcome_screen.png?raw=true)  
Insert a card **the wrong way around** to get to this screen:  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/rf_debug_guide/invalid.png?raw=true)  
In Moolticute, press CTRL-SHIFT-F3 (SHIFT-Command-F3 on Mac) in such a way that all 3 keys are pressed at the end of the shortcut.  
The BLE Dev tab will appear on the top part of the application. Click on it:  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/minible_update_guide/ble_dev_tab_upload_flash.png?raw=true)  
Click on the "Select Bundle File" button, select the bundle file we sent you by email.  
The upload will start automatically:  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/minible_update_guide/firmware_file_update.png?raw=true)  
At the end of the upload, click on **Update platform**.  
The platform will then reboot and update itself.  
**Do not disconnect your device during the update process**.  
