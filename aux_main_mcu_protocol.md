## [](#header-1) Communications Between Aux and Main MCU
The auxiliary and main microcontrollers can communicate with each other using a serial link.  
While the main role of the auxiliary MCU is to receive/send data packets from/to USB or Bluetooth, other types of messages (firmware update, keyboard HID presses) are also needed.  
  
## [](#header-2) Message Structure and Serial Link Specs 
The fixed length **544 bytes long** message structure is defined as follows:  

from aux MCU:   
  
| byte 0 - 1   | byte 2 - 3        | byte 4 - 539  | byte 540 - 541    | byte 542 - 543      |
|:-------------|:------------------|:--------------|:------------------|---------------------|
| Message Type | Payload Length #1 | Payload       | Payload Length #2 | Payload valid flag  |

from main MCU:  
    
| byte 0 - 1   | byte 2 - 3        | byte 4 - 539  | byte 540 - 541    | byte 542 - 543      |
|:-------------|:------------------|:--------------|:------------------|---------------------|
| Message Type | Payload Length    | Payload       | Reserved          | Reserved            |
  
**Message Type**  
- 0x0000: Message to/from USB  
- 0x0001: Message to/from Bluetooth  
- 0x0002: Message to/from Aux MCU Bootloader 
  
**Payload Length**  
Message from main MCU: total payload length.  

**Payload Length #1**  
Message from aux MCU: if different than 0, total payload length. Otherwise payload length #2 is the actual payload length.  
  
**Payload Length #2**  
Message from aux MCU: if payload length #1 is 0, total payload length. Otherwise discarded.  
  
**Payload**   
The message payload, which may contain up to 536 bytes. This maximum size was decided in order to accomodate a single "write data node" command (command identifier: 2 bytes, message payload size field: 2 bytes, data node address: 2 bytes, data node size: 528 bytes and 2 additional bytes reserved).
  
**Payload valid flag**  
This field should only be taken into account if **payload length #1 is 0**.  
If different than 0, this byte lets the message recipient know that the message is valid. As a given Mooltipass message sent through USB can be split over multiple 64 bytes packets, this byte allows the aux MCU to signal that this message is invalid if for some reason or another the sequence of 64bytes long HID packets sending is interrupted.
  
**Serial Link Specs**  
The current USART baud rate clock is set to **6MHz**.  
They key performance metric we want to hit is being able to scan the external DB flash memory as fast as possible (used when entering credentials management mode). That means the AUX MCU should have the first 64 bytes to send to the computer within 1ms after receiving the read node command (which itself is less than 64B long).  
A first unsuccessful approach to hit that goal was to use linked descriptors. However, due to errata 15683 for ATSAMD21 MCUs it is impossible to use them.  
As a result, the main loop on both MCUs will continuously read the ongoing receive transfer DMA byte count. Therefore, the main MCU can process a "read node" command as soon as 2 (message type) + 2 (payload length #1) + 2 (command identifier) + 2 (payload length) + 2 (node address) = 10 bytes are received in 17us and the aux MCU could start sending data after the first 64 bytes are received in 106us.  
  
## [](#header-2) Not Losing Packets
Three different kinds of packets may be sent from the AUX MCU:  
- USB communications  
- BLE communications  
- status messages (USB disconnected & others)  

If no flow control was implemented these 3 different packets may be sent **at once** to the main MCU, which may not have enough time to deal with each packet before being able to receive another.  
As a consequence, from the "proto v2" boards, a dedicated main MCU output pin explicitely lets the aux MCU know to not send any packet. This does not lead to additional memory requirements on the aux MCU as:   
- a USB buffer needs to be implemented for packet de-serialization  
- same for the BLE buffer  
- the status messages are generated on the fly  



