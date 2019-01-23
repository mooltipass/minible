## Aux-Main MCU Messages for Aux MCU Firmware Upgrade

The protocol for updating the Aux MCU firmware has been designed to be as simple as possible. This has been made possible by the fact that the Aux MCU is considered insecure.  
Here is a quick overview of how the update process works:  
1. Main MCU main firmware sends a BOOTLOADER_START_PROGRAMMING_COMMAND messsage to Aux MCU  
2. Aux MCU reboots into bootloader, sends BOOTLOADER_START_PROGRAMMING_COMMAND ack message  
3. Main MCU sends 512B long firmware data chunks in BOOTLOADER_WRITE_COMMAND messages  
4. Main MCU sends BOOTLOADER_START_APP_COMMAND message to Aux MCU  
5. Aux MCU reboots into its upgraded firmware  
  
  
## Message Structure & Commands

The messages used are a subset of [Aux Mcu <-> Serial Link Specification](aux_main_mcu_protocol.md), where the message type is __0x0002__. The sub-messages described below are therefore embedded starting at byte 4 of the message format described in the previous link.

#### Enter Programming Command (0x0000)

From Main MCU:  

| byte 0-1 | byte 2-5 | byte 6-9 |
|:-:|:-:|:-:|
| 0x0000 | Image Length | Image CRC (not implemented) |

The Aux MCU bootloader sends the same packet without the image length & crc fields completed to acknwledge bootloader start.


#### Write Command (0x0001)

| byte 0-1 | byte 2-5 | byte 6-9 | byte 10-13 | byte 14-525 |
|:-:|:-:|:-:|:-:|:-:|
| 0x0001 | payload size (512) | payload CRC (not implemented) | write address | 512B payload |

- __Command__: 0x0001
- __Size__: Number of data bytes: only 512 bytes (2 rows) are supported at the moment
- __CRC__: CRC of Data. (not yet implemented)
- __Data__: Data to flash, it shall contain the application.
- On every Write command, erase row will previously be executed before and will erase 4 pages (64B each page).
- The internal address to write will be incremented on each write command.
- After last write is performed, the bootloader will jump directly to application.

#### Aux Mcu Bootloader to Main Mcu (Response)
1. Echo as positive response, next command can be received
2. Full payload to 0xFF as negative response, main mcu shall reset the bootlooder and try again.

Main MCU shall wait for the answer of Aux MCU bootloader to transmit the next command
