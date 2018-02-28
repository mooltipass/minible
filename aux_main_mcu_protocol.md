## [](#header-1) Communications Between Aux and Main MCU
The auxiliary and main microcontrollers can communicate with each other using a serial link.  
While the main role of the auxiliary MCU is to receive/send data packets from/to USB or Bluetooth, other types of messages (firmware update, keyboard HID presses) are also needed.  
  
## [](#header-2) Message Structure and Serial Link Specs 
The message structure is defined as follows:  

| byte 0 - 1   | byte 2 - 3       | byte 4 - up to 535  |
|:-------------|:-----------------|---------------------|
| Message Type | Payload Length   | Payload             |

**Message Type**  
- 0x0000: Message to/from USB  
- 0x0001: Message to/from Bluetooth  
  
**Payload Length**  
Total payload length.  
  
**Payload**   
The message payload, which may contain up to 532 bytes. This maximum size was decided in order to accomodate a single "write data node" command (command identifier: 2 bytes, message payload size field: 2 bytes, data node size: 528 bytes).
  
**Serial Link Specs**  
Ideally, we'd like to start answering any message within 1ms (minimum HID send window). On a purely link layer, this means sending 536x2 bytes in less than one ms (1.07Mbit/s).  
As MCUs are running at 48MHz and given we use integer clock dividers, the current serial clock is set to 3MHz (max value for 16x oversampling).
