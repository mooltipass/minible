## [](#header-2)Thanks for Testing our New Device!  
Thanks for embarking with us in this adventure. The device you have in your hands is the fruit of 2 years of hard labour and only **you** can help us make sure that it is ready to be sent to other Mooltipass enthusiasts!  
Your mission, if you choose to accept it, is to share with us your experience with your Mooltipass Mini BLE:
- what do you like?  
- what frustrates you?  
- what do you wish was implemented?  
- what would you do differently?  
- what is completely broken?  
  
    
    
### [](#header-3)Current Capabilities & Installation Process  
To get started, install the Mooltipass extension for the browsers you may be using:  
- <a href="https://chrome.google.com/webstore/detail/mooltipass-extension/ffemldjbbfhimggdkkckolidemlllklk">Chrome</a>  
- <a href="https://addons.mozilla.org/en-US/firefox/addon/mooltipass-extension/">Firefox</a>  
- <a href="https://addons.opera.com/en/extensions/details/mooltipass-extension/">Opera</a>  
  
Then install the latest version of Moolticute, which can be found <a href="https://betas.themooltipass.com/">here</a>.  
For ubuntu, you may use https://launchpad.net/~mooltipass/+archive/ubuntu/moolticute-beta  
   
The device you received is fully functional. It will allow you to:  
- store and recall website credentials using our browser extensions  
- manually recall credentials through USB and Bluetooth using the login & favourite menus   
- fully manage your credentials in Moolticute  
- show your credentials on the display  
  
To speed-up development efforts, we unfortunately haven't written a user manual (yet). We however believe this may be a good thing, as we expect you to try the Mooltipass BLE in a way that feels intuitive to you, and then tell us if it is not the case.  
  
  
  
### [](#header-3)Battery Autonomy  
You'll find that the battery autonomy mainly depends on the OLED screen ON time. The device, after a couple of seconds, will directly go to sleep and will stay alive:  
- 100 days when Bluetooth is OFF    
- 35 days when Bluetooth is ON  
- 10 days when Bluetooth is ON and connected  
  
These numbers are purely theoretical and will need to be confirmed by your experience!  
  
  
  
### [](#header-3)Basic Controls   
As with the Mooltipass Mini, the Mooltipass Mini BLE is fully operated with a clickable scroll wheel: scroll to navigate through the menus / prompts, click to approve and long press to cancel / go back.  
In the (un)likely case your device becomes unresponsive, you may force a shutdown by removing the USB cable attached to it, then keeping the wheel pressed for at least 10 seconds.  
A debug menu is also available to (among others) switch off the device. We strongly recommend to not try it but if you wish so it can be accessed by removing any smartcard inserted in the device, waiting for the main screen to be showed and then keeping the wheel pressed for a couple of seconds.  
  
  
  
### [](#header-3)Known Bugs and Limitations  
The following bugs and limitations are known and currently being worked on:  
1) **Limited Bluetooth pairing capabilities**  
The add/remove items in the Bluetooth menu aren't implemented yet and only one device can be paired to the Mooltipass. To pair a bluetooth device, simply enable Bluetooth on the mooltipass and then pair your device with it. The connection status is shown in the status bar in the bottom right corner of the screen. If you wish to pair another device, you'll need to power cycle the device using the debug menu.  
2) **No USB communication when connected through a hub on Windows**  
This is an odd one that we hope to fix soon. Please let us know if you experience this issue.  
  
  
  
### [](#header-3)Simple and Advanced Modes  
After creating a new user, the Mooltipass Mini BLE will prompt you to enable simple mode.  
If you deny this prompt, the device will then allow you to configure the following settings:  
- prompt you whenever you want to login to a website  
- ask for your PIN when switching to management mode
- prompt to show credentials when using the login or favourites menu  
- prompt to save credentials when exiting management mode  
- change the device language for the current user  
- change the keyboard layouts for the current user  

As you can guess, we now allow users to customize the security / user-friendliness tradeoff. In simple mode, the device will simply send credentials automatically whenever they are needed and automatically store credentials when you modify them in memory management mode in Moolticute.  
You may still switch between simple and advanced mode by going into the operations menu on the device.  
  
  
  
### [](#header-3)Should my Device be Considered Secure?  
**Kind of**.  
Starting from main MCU firmware v0.7 firmware updates are only allowed when you insert your card inside the mini BLE the wrong way around. This will guarantee that your device can't be modified without someone having physical access to it.  
  
  
  
### [](#header-3)How to Report Bugs  
You will inevitably find bugs on the device you received... it's part of the hardware/firmware development process after all!  
To make sure the development team is able to efficiently fix them, please follow these guidelines:  
- devise a 'recipe' that others may follow to replicate your issue  
- if the bug is software (Moolticute) related, report it <a href="https://github.com/mooltipass/moolticute/issues/new">here</a>  
- if the bug is hardware related, report it <a href="https://github.com/mooltipass/minible/issues/new">there</a>  
- if you want to report a broader issue (non-intuitive user interface, odd general device behavior), please use our <a href="https://groups.google.com/forum/#!forum/mooltipass-ble-beta-testers">google group</a>  

Please make sure you have the full developper log feature enabled in Moolticute. To enable it, simply head to the settings tab, scroll down to tick the "Enable Debug Log" checkbox and then restart Moolticute by closing it (right click on the Moolticute icon in the task bar, "quit") and launching it again. The logs can then be found by clicking the "View" button in Moolticute's settings tab.  



### [](#header-3)Updating Moolticute  
Your computer should notify you whenever we release a Moolticute update. If that's not the case, you may simply click the "check for Updates" button in Moolticute's "About" tab:  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/moolticute_update_guide/check_for_update.PNG?raw=true)  
If everything goes well, you'll see a prompt for update at the bottom of the window:
![](https://github.com/mooltipass/minible/blob/gh-pages/images/moolticute_update_guide/update_message.PNG?raw=true)  
Simply click yes and go through the update process.  



### [](#header-3)Updating your Device  
Insert your smarcard **the wrong way around** inside the device, make sure you're connecting through USB.  
Connect your device to your computer **using USB**, make sure Bluetooth connectivity is disabled.  
In Moolticute, press CTRL-SHIFT-F3 (SHIFT-Command-F3 on Mac) in such a way that all 3 keys are pressed at the end of the shortcut. The BLE Dev tab will appear:  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/minible_update_guide/ble_dev_tab_oneclick.PNG?raw=true)  
Click on the "Select Bundle File" button and select the bundle file we sent you by email, the upload will start automatically.  
Once the upload is performed, **click the Update Platform button**. The BLE Dev tab will then disappear and you'll hear the device disconnecting and reconnecting from USB.  
