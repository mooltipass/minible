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
  
From Main MCU:   
  
| byte 0-1 | byte 2-5 | byte 6-9 | byte 10-13 | byte 14-525 |
|:-:|:-:|:-:|:-:|:-:|
| 0x0001 | payload size (512) | payload CRC (not implemented) | write address | 512B payload |

On every Write command, erase row will previously be executed before and will erase 4 pages (64B each page).
The Aux MCU bootloader sends the same packet without the non command fields completed to acknwledge data write.


#### Start App Command (0x0002)
  
From Main MCU:   
  
| byte 0-1 |
|:-:|
| 0x0002 |

Aux MCU will boot into its upgraded firmware and won't send a reply packet.
