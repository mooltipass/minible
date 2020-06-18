## [](#header-2)Thanks for Helping Us Create a Multi-Language Mooltipass !  
The Mooltipass Mini BLE is the first Mooltipass device to include multiple languages, together with Unicode support.  
For an easier translation process, you'll be able to test your translations on **a device emulator** on **your computer**.  

## [](#header-2)Windows
  
### [](#header-3)Prerequisites
To start your work, you'll need to:  
- Install <a href="https://www.themooltipass.com/setup/">Moolticute</a>    
- Install <a href="https://www.python.org/ftp/python/2.7.18/python-2.7.18.amd64.msi">Python2</a>  
- Download the zip file sent to you by email  

### [](#header-3)Launching the Emulator
- Open a file explorer and navigate to C:\Users\your user name\AppData\Local\Programs\Moolticute  
- Double click on minible_emu.exe  

### [](#header-3)Changing the Device Strings
The device emulator uses the "miniblebundle.img" file located inside Moolticute's installation folder to fetch the strings displayed to the user. To change these strings, you'll therefore need to generate a new miniblebundle.img file:  
- Unzip the folder sent to you by email  
- Change the contents of "0_EN_US_english_strings.txt" inside the "strings" folder
- Double click on "bundle.py"  
- Overwrite the miniblebundle.img file located inside Moolticute's install folder with the one that was just generated.  
- Start the emulator  
Before overwriting miniblebundle.img, do not forget to close both windows opened by the device emulator.  


## [](#header-2)Ubuntu
  
### [](#header-3)Prerequisites
- Install <a href="https://launchpad.net/~mooltipass/+archive/ubuntu/moolticute">Moolticute</a>  
- Install the <a href="https://launchpad.net/~mooltipass/+archive/ubuntu/minible-beta">Mooltipass Mini BLE Emulator</a>  
- Install Python2 by typing "sudo apt install python2" in a terminal  

### [](#header-3)Launching the Emulator
- Open a terminal and type "minible"  
- If not already launched, you may also type "moolticute" to lauch Moolticute  

### [](#header-3)Changing the Device Strings
The device emulator uses the "miniblebundle.img" file located inside /share/misc to fetch the strings displayed to the user.   
To change these strings, you'll therefore need to generate a new miniblebundle.img file:  
- Unzip the folder sent to you by email  
- Change the contents of "0_EN_US_english_strings.txt" inside the "strings" folder  
- In a terminal, type "python2 bundle.py"  
- Overwrite the miniblebundle.img file located in /share/misc with the one that was just generated  
- Start the emulator  
Before overwriting miniblebundle.img, do not forget to close both windows opened by the device emulator.  

## [](#header-2)Notes
As you'll have noticed by now, the device contains around 125 strings. All of them (except maybe 10) can be displayed on the emulator screen following the indications shown in "0_EN_US_english_strings.txt".  
To review your translations, we would appreciate if you could perhaps save screen captures of your translations by using a simple software such as <a href="https://clipclip.com/">clipclip</a> on Windows to automatically save window captures taken with the Alt-PrtScrn shortcut.

