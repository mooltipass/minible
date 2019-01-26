## [](#header-1) USB HID Protocol
To accomodate its new features, the new Mooltipass Mini uses a new protocol implemented on top of 64 bytes HID packets. It is **not** compatible with previous protocols.   
All communications are initiated by the computer.  
   
### [](#header-3) High Level HID Packet Structure Overview

| byte 0                                            | byte 1                      | bytes 2-63 |
|:--------------------------------------------------|:----------------------------|:-----------|
| message flip bit, ack flag, packet payload length | packet ID for given message | payload    |
   
### [](#header-3) Byte 1 Description

| bits 7 to 4                 | bits 3 to 0                 |
|:----------------------------|:----------------------------|
| packet ID for given message | total number of packets - 1 |
  
**Example:**  
If the computer wants to send a 240 bytes long message to the mini, it will send **4** raw HID packets whose bytes 1 are:  
- computer packet #0: 0x03  
- computer packet #1: 0x13  
- computer packet #2: 0x23  
- computer packet #3: 0x33  
   
### [](#header-3) Byte 0 Description

| bit 7            | bit 6                             | bits 5 to 0                   |
|:-----------------|:----------------------------------|:------------------------------|
| message flip bit | final acknowledge flag or request | current packet payload length |
  
The message flip bit's main purpose is to explicitely mention that a new **message** (not packet) is being sent. This also allows the computer to discard a half-sent message by sending a new one with this bit flipped.  
Therefore, this bit should be flipped every time a new message is sent. This bit shouldn't be used when received by the computer.   
The mooltipass device will discard packets having the flip bit incorrectly set. To reset the flip bit state machine, the computer may simply send a packet with **the first two bytes set to 0xFF**. The device will then expect the next packet to have the flip bit set to 0.  
At the end of a message transmission, if the computer set the final acknowledge request flag (bit6), regardless of the message contents' expected answer, the device will answer with a single 64B packet with the same byte0 and byte1 and the final acknowledge flag set.  
This allows the computer to make sure his message was received.  
  
**Example:**  
If the computer wants to send **2** 240 bytes long messages to the mini, it can send **8** raw HID packets whose first two bytes are:  
- computer packet #0: 0x40 + 62 (base 10) 0x03    
- computer packet #1: 0x40 + 62 (base 10) 0x13     
- computer packet #2: 0x40 + 62 (base 10) 0x23      
- computer packet #3: 0x40 + 54 (base 10) 0x33   
- device packet #0: 0x40 + 54 (base 10) 0x33 
- packet #4: 0x80 + 0x40 + 62 (base 10) 0x03    
- packet #5: 0x80 + 0x40 + 62 (base 10) 0x13     
- packet #6: 0x80 + 0x40 + 62 (base 10) 0x23      
- packet #7: 0x80 + 0x40 + 54 (base 10) 0x33    
- device packet #1: 0xC0 + 54 (base 10) 0x33
  
### [](#header-3) Protocol Limitations
- maximum message size is 62*16= **992 bytes**  
- no message integrity checks are performed (implemented at the USB layer)
   
### [](#header-3) Backward Compatibility Implications
This protocol has been designed to **not** be compatible with our previous one.  
In our previous protocol, all byte1 include numbers **above 0xA0**. For this new protocol, this would mean a packet ID equal to 10 without having received the previous ones.  

### [](#header-3) Checks to be implemented on the device
- check for sequential packet numbers  
- check that the current packet number isn't above the total number of expected packets  
- check for message bit flip between 2 successive **messages**  
- once a message is received, if the final acknowledge request flag is set, reply with a **single 64B packet** with the same byte0 and byte1 and final acknowledge flag set  
