## [](#header-1) Mooltipass Protocol
This page explains the new Mooltipass messaging protocol and the different commands implemented in its firmware.
   
## [](#header-2) High Level Message Structure Overview

| byte 0             | byte 1-2       | bytes 3-X |
|:-------------------|:---------------|:----------|
| Command Identifier | Payload Length | Payload   |
   
As with our previous devices, every non-debug / non-test message sent by the computer will receive an answer.  
In all command descriptions below, hmstrlen() is a custom strlen function treating a character as a uint16_t (Unicode BMP). Therefore, hmstrlen("a") = 2.  
  
## [](#header-2) Mooltipass Commands

0x00: Debug Message
-------------------

| byte 0 | byte 1-2                    | bytes 3-X     |
|:-------|:----------------------------|:--------------|
| 0x00   | hmstrlen(debug_message) + 1 | Debug Message |

Can be sent from both the device or the computer. **Does not require an answer.** 
