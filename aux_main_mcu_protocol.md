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
- 0x0002: Message to/from Bootloader 
  
**Payload Length**  
Message from main MCU: total payload length.  

**Payload Length #1**  
Message from aux MCU: if different than 0, total payload length. Otherwise payload length #2 is the actual payload length.  
  
**Payload Length #2**  
Message from aux MCU: if payload length #0 is 0, total payload length. Otherwise discarded.  
  
**Payload**   
The message payload, which may contain up to 536 bytes. This maximum size was decided in order to accomodate a single "write data node" command (command identifier: 2 bytes, message payload size field: 2 bytes, data node address: 2 bytes, data node size: 528 bytes).
  
**Payload valid flag**  
This field should only be taken into account if **payload length #1 is 0**.  
If different than 0, this byte lets the message recipient know that the message is valid. As a given Mooltipass message sent through USB can be split over multiple 64 bytes packets, this byte allows the aux MCU to signal that this message is invalid if for some reason or another the sequence of 64bytes long HID packets sending is interrupted.
  
**Serial Link Specs**  
The current USART baud rate clock is set to **6MHz**.  
They key performance metric we want to hit is being able to scan the external DB flash memory as fast as possible (used when entering credentials management mode). That means the AUX MCU should have the first 64 bytes to send to the computer within 1ms after receiving the read node command (which itself is less than 64B long).   
Doing so involves the following techniques:  
- DMA descriptor chaining on main MCU: 2 + 2 + 64 = 68B and 544 - 68 = 476B  
- after first DMA descriptor interrupt, DB flash data fetch  
- same DMA descriptor chaining on aux MCU  
  
This leads to a theoretical transfer rate of 68x2=136B per ms (**1.36MHz** baud rate due to start & stop bits). As we however do prefer only using 2 chained DMA descriptors, this involves transferring a total of 68+544B within 2ms, leading to a USART baud rate requirement of (68+544)x500x10x8/8 = **3.06MHz**. Hence the selected 6MHz baud rate clock.  
Please note that this thinking also applies to a requirement of being able to send pings every 2ms.  
  
**Linked Descriptors on ATSAMD21**  
In reality, it is not possible to use linked DMA descriptors because of errata 15683. As a mitigation technique, in our main while loop we check the ongoing receive transfer DMA transferred byte count.
