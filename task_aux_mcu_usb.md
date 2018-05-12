**Title**: Modify or rewrite the current AUX MCU USB communication layer  
**Time estimated**: medium  
**Skills**: C  
**Description**:  
Some features are missing at the USB layer and need to be implemented. Moreover, no proper research was done to find a proper USB stack.  
**Tasks**:  
- Read [our USB protocol documentation](usb_hid_protocol)   
- Understand our current code  
- Implement the missing USB features such as payload #1 and #2  
- Check if a non-ASF USB stack would be more efficient and switch to it if needed  
