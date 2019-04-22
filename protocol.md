## [](#header-1) Mooltipass Protocol
This page explains the new Mooltipass messaging protocol and the different commands implemented in its firmware.
   
## [](#header-2) High Level Message Structure Overview

| byte 0-1           | byte 2-3       | bytes 4-X |
|:-------------------|:---------------|:----------|
| Command Identifier | Payload Length | Payload   |
   
Debug and test commands have identifiers above 0x8000.  
As with our previous devices, every non-debug / non-test message sent by the computer will receive an answer.  
In all command descriptions below, hmstrlen() is a custom strlen function treating a character as a uint16_t (Unicode BMP). Therefore, hmstrlen("a") = 2.  
Unless written otherwise, all commands below have been tested on an actual device.  
  
## [](#header-2) Mooltipass Commands

0x0001: Ping
------------

From the PC:

| byte 0-1 | byte 2-3         | bytes 4-X         |
|:---------|:-----------------|:------------------|
| 0x0001   | up to the sender | Arbitrary payload |

The device will send back the very same message.  


0x0002: Please Retry
--------------------

From the device:

| byte 0-1 | byte 2-3         | bytes 4-X         |
|:---------|:-----------------|:------------------|
| 0x0002   | 0                | N/A               |

When the device is busy and can't deal with the message sent by the computer, it will reply a message with a "Please Retry" one, inviting the computer to re-send its packet.  


0x0003: Get Platform Info
-------------------------

From the PC:

| byte 0-1 | byte 2-3         | bytes 4-X         |
|:---------|:-----------------|:------------------|
| 0x0003   | 0                | N/A               |

Device answer:

| bytes  | value  |
|:-------|:-------|
| 0->1   | 0x0003 |
| 2->3   | 14     |
| 4->5   | Main MCU fw major |
| 6->7   | Main MCU fw minor |
| 8->9   | Aux MCU fw major |
| 10->11 | Aux MCU fw minor |
| 12->15 | Platform serial number |
| 16->17 | DB memory size |


0x0004: Set Current Date
------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | bytes 4-5                          | bytes 6-X         |
|:---------|:----------------------------|:-----------------------------------|:------------------|
| 0x0004   | 2 | current date (15 dn 9 -> Year (2010 + val), 8 dn 5 -> Month, 4 dn 0 -> Day of Month) | N/A |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0004   | 1 | 0x01 (indicates command success) |


0x0005: Cancel Request
------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0005   |        N/A   |

No device answer.

Tested status: NOT tested


0x0006: Store Credential
------------------------

From the PC: 

| bytes  | value  |
|:-------|:-------|
| 0->1   | 0x0006 |
| 2->3   | depends on message contents |
| 4->5   | index to the service name (0) |
| 6->7   | index to the login name or 0xFFFF |
| 8->9   | index to the description or 0xFFFF |
| 10->11 | index to the third field or 0xFFFF |
| 12->13 | index to the password or 0xFFFF |
| 14->xxx | all above no 0xFFFF terminated fields concatenated |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0005   | 1 | 0x01 or 0x00 (success or fail) |


0x0007: Get Credential
----------------------

From the PC: 

| bytes  | value  |
|:-------|:-------|
| 0->1   | 0x0007 |
| 2->3   | depends on message contents |
| 4->5   | index to the service (0) |
| 6->7   | index to the login or 0xFFFF |
| 8->xxx | all above no 0xFFFF terminated fields concatenated |

Device Answer:

| bytes  | value  |
|:-------|:-------|
| 0->1   | 0x0007 |
| 2->3   | 0 for fail, otherwise depends on message contents |
| 4->5   | index to the login name |
| 6->7   | index to the description |
| 8->9   | index to the third field |
| 10->11 | index to the password |
| 12->xxx | all above 0 terminated fields concatenated |


0x0008: Get 32 Random Bytes
---------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0008   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3 | byte 4-35 |
|:---------|:---------|:----------|
| 0x0008   | 32       | Random bytes |


0x0009: Start Memory Management
-------------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0009   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0009   | 1 | 0x00 (failure) / 0x01 (success) |


0x000A: Get User Change Numbers
-------------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x000A   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4-7  | byte 8-11 |
|:---------|:----------------------------|:----------|:----------|
| 0x000A   | 8 | Credential change number | Data change number |

Tested status: NOT tested


## [](#header-2) Memory Management Commands

If any of the commands below are sent when the device isn't in memory management mode, the reply will be a single 0x00 byte.


0x0100: Get Start Nodes Addresses
---------------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0100   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0100   | 34 | Credential start address + 16 data start addresses |

Tested status: NOT tested


0x0101: Leave Memory Management
-------------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0101   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0101   | 1 | 0x00 (failure) / 0x01 (success) |


0x0102: Read Memory Node
------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0102   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0102   | 264 or 528 | Memory node contents |

Tested status: NOT tested


0x0103: Set User Change Numbers
-------------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | byte 4-7  | byte 8-11 |
|:---------|:----------------------------|:----------|:----------|
| 0x0103   | 8 | Credential change number | Data change number |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0103   | 1 | 0x00 (failure) / 0x01 (success) |

Tested status: NOT tested


  
## [](#header-2) Mooltipass Debug and Test Commands

0x8000: Debug Message
-------------------

| byte 0-1 | byte 2-3                    | bytes 4-X                          |
|:---------|:----------------------------|:-----------------------------------|
| 0x8000   | hmstrlen(debug_message) + 2 | Debug Message + terminating 0x0000 |

Can be sent from both the device or the computer. **Does not require an answer.** 


0x8001: Open Display Buffer
---------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | bytes 4-X                          |
|:---------|:----------------------------|:-----------------------------------|
| 0x8001   | 0 | Nothing |

Open the oled display buffer for writing.

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x8001   | 1 | 0x01 (indicates command success) |



0x8002: Send Pixel Data to Display Buffer
---------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | bytes 4-X                          |
|:---------|:----------------------------|:-----------------------------------|
| 0x8002   | Payload size = number of pixels x 2 | Pixel data |

Send raw display data to opened display buffer. 

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x8002   | 1 | 0x01 (indicates command success) |



0x8003: Close Display Buffer
---------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | bytes 4-X                          |
|:---------|:----------------------------|:-----------------------------------|
| 0x8003   | 0 | Nothing |

Stop ongoing display buffer data writing.

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x8003   | 1 | 0x01 (indicates command success) |



0x8004: Erase Data Flash
------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | bytes 4-X                          |
|:---------|:----------------------------|:-----------------------------------|
| 0x8004   | 0 | Nothing |

Fully erase data flash. Returns before the flash is erased.

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x8004   | 1 | 0x01 (indicates command success) |



0x8005: Query Dataflash Ready Status
------------------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | bytes 4-X                          |
|:---------|:----------------------------|:-----------------------------------|
| 0x8005   | 0 | Nothing |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x8005   | 1 | 0x00 (dataflash busy) or 0x01 (dataflash ready) |



0x8006: Write 256 Bytes to Dataflash
------------------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | bytes 4-7                          | bytes 8-263                        |
|:---------|:----------------------------|:-----------------------------------|:-----------------------------------|
| 0x8006   | 260 | Write address | 256 bytes payload |

As it is a debug command, no boundary checks are performed. The place at which these 256 bytes are written should be previously erased.

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x8006   | 1 | 0x01 (indicates command success) |



0x8007: Reboot main MCU to Bootloader
-------------------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | bytes 4-X |
|:---------|:----------------------------|:----------|
| 0x8007   | 0 | Nothing |

Start main microcontroller bootloader. **No device answer**.



0x8008: Get 32 Samples of Accelerometer Data
--------------------------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | bytes 4-X |
|:---------|:----------------------------|:----------|
| 0x8008   | 0 | Nothing |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x8008   | 192 | 32x (accX, accY, accZ - uint16_t) |



0x8009: Flash Aux MCU with Bundle Contents
------------------------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | bytes 4-X |
|:---------|:----------------------------|:----------|
| 0x8009   | 0 | Nothing |

Flash aux MCU with binary file included in the bundle. **No device answer**.



0x800A: Get Platform Info
-------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | bytes 4-X |
|:---------|:----------------------------|:----------|
| 0x800A   | 0 | Nothing |

Request platform info

Device Answer:

| Bytes | Description |
|:------|:------------|
| 0-1   | 0x800A |
| 2-3   | payload length (TBD) |
| 4-55  | see [here](aux_platform_spec_message) |
| 56-67 | reserved |
| 67-68 | main MCU fw version, major |
| 69-70 | main MCU fw version, minor |



0x800B: Reindex Bundle
------------------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | bytes 4-X                          |
|:---------|:----------------------------|:-----------------------------------|
| 0x800B   | 0 | Nothing |

Ask the platform to reindex the bundle (used after uploading a new bundle).

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x800B   | 1 | 0x01 (indicates command success) |
