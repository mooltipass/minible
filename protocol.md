## [](#header-1) Mooltipass Protocol
This page explains the new Mooltipass messaging protocol and the different commands implemented in its firmware.
   
## [](#header-2) High Level Message Structure Overview

| byte 0             | byte 1-2       | bytes 3-X |
|:-------------------|:---------------|:----------|
| Command Identifier | Payload Length | Payload   |
   
As with our previous devices, every message sent by the computer will receive an answer.  
  
### [](#header-3) Mooltipass Commands

0x00: Debug Message
-------------------
From Mooltipass: packet data containing the debug message.   
