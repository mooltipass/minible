**Title**: Automatically fetch all UI features from device  
**Time estimated**: medium  
**Skills**: python & C  
**Description**:  
Remotely fetch from USB the contents of the display frame buffer and implement remote control of the device (within #defines). This will therefore allow:  
- Making sure all language strings are correctly displayed on the device  
- Ease development time as developers won't have to manually operate the device  
- The creation of a "virtual" visual device as screens could be manually navigated through  
   
**Tasks**:  
- USB: implement a debug fetch buffer command  
- USB: implement a wheel moved / card inserted command  
- python: export pictures  
- python: generate a list of actions in order to reach every device screen  
- TBD: generate a tool to create the virtual visual device from the screens fetched  
