## [](#header-1) Communications Between Aux and Main MCU
The auxiliary and main microcontrollers can communicate with each other using a serial link.  
While the main role of the auxiliary MCU is to receive/send data packets from/to USB or Bluetooth, other types of messages (firmware update, keyboard HID presses) are also needed.  
  
## [](#header-2) Message Structure and Serial Link Specs 
The message structure is defined as follows:  

| byte 0 - 1   | byte 2 - 535  | byte 536 - 537   | byte 538            |
|:-------------|---------------|:-----------------|---------------------|
| Message Type | Payload       | Payload Length   | Payload valid flag  |

**Message Type**  
- 0x0000: Message to/from USB  
- 0x0001: Message to/from Bluetooth  
  
**Payload Length**  
Total payload length.  
  
**Payload**   
The message payload, which may contain up to 534 bytes. This maximum size was decided in order to accomodate a single "write data node" command (command identifier: 2 bytes, message payload size field: 2 bytes, data node address: 2 bytes, data node size: 528 bytes).
  
**Payload valid flag**
If different than 0, this byte lets the message recipient know that the message is valid. As a given Mooltipass message sent through USB can be split over multiple 64 bytes packets, this byte allows the aux MCU to signal that this message is invalid if for some reason or another the sequence of 64bytes long HID packets sending is interrupted.
  
**Serial Link Specs**  
Ideally, we'd like to start answering any message within 1ms (minimum HID send window). As the aux MCU can start streaming data the moment it receives it (through USB or Bluetooth), this means that at least 128 bytes (64bytes back and forth) needs to be sent within 1ms. Taking into account the USART start & stop bits, our required baud rates becomes 128x8x8/10/0.001 = 820kHz.  
We therefore decided to use **1MHz**.
