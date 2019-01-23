## Messages for Aux MCU Firmware Upgrade

The protocol for updating the Aux MCU firmware has been designed to be as simple as possible. This has been made possible by the fact that the Aux MCU is considered insecure.  
Here is a quick overview of how the update process works:  
1. Main MCU firmware sends a start programming messsage to Aux MCU  
2. Aux MCU reboots into bootloader, sends an ack message  
3. Main MCU sends acknowledged 512B long firmware data chunks  
4. Main MCU sends start app message to Aux MCU  
5. Aux MCU reboots into its upgraded firmware  
  
  
## Message Structure & Commands

The messages used are a subset of [Aux Mcu <-> Serial Link Specification](aux_main_mcu_protocol.md), where the message type is __0x0002__. The sub-messages described below are therefore embedded starting at byte 4 of the message format shown in the previous link.

#### Enter Programming Command (0x0000)

From Main MCU:  

| byte 0-1 | byte 2-5 | byte 6-9 | byte 10-535 |
|:-:|:-:|:-:|:-:|
| 0x0000 | Image Length | Image CRC (not implemented) | DNC |

The Aux MCU bootloader sends the same packet without the image length & crc fields completed to acknowledge bootloader start.


#### Write Command (0x0001)
  
From Main MCU:   
  
| byte 0-1 | byte 2-5 | byte 6-9 | byte 10-13 | byte 14-525 | byte 526-535 |
|:-:|:-:|:-:|:-:|:-:|:-:|
| 0x0001 | payload size (512) | payload CRC (not implemented) | write address | 512B payload | DNC |

When receiving this message, the Aux MCU will take care of erasing its rows at the specified addresss. The Aux MCU bootloader then sends the same packet without the non command fields completed to acknowledge data write.  


#### Start App Command (0x0002)
  
From Main MCU:   
  
| byte 0-1 | byte 2-535 |
|:-:|:-:|
| 0x0002 | DNC |

Aux MCU will boot into its upgraded firmware and won't send a reply packet.
