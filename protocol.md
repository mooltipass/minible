## [](#header-1) Mooltipass Protocol
This page explains the new Mooltipass messaging protocol and the different commands implemented in its firmware.
   
## [](#header-2) High Level Message Structure Overview

| byte 0-1           | byte 2-3       | bytes 4-X |
|:-------------------|:---------------|:----------|
| Command Identifier | Payload Length | Payload   |
   
Debug and test commands have identifiers above 0x8000.  
As with our previous devices, every non-debug / non-test message sent by the computer will receive an answer.  
In all command descriptions below, hmstrlen() is a custom strlen function treating a character as a uint16_t (Unicode BMP). Therefore, hmstrlen("a") = 2.  
  
## [](#header-2) Mooltipass Commands

0x0001: Ping
------------

| byte 0-1 | byte 2-3         | bytes 4-X         |
|:---------|:-----------------|:------------------|
| 0x0001   | up to the sender | Arbitrary payload |

The device will send back the very same message.

  
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

| byte 0-1 | byte 2-3                    | bytes 4-X                          |
|:---------|:----------------------------|:-----------------------------------|
| 0x8005   | 0 | Nothing |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x8005   | 1 | 0x00 (dataflash busy) or 0x01 (dataflash ready) |



0x8006: Write 256 Bytes to Dataflash
------------------------------------

| byte 0-1 | byte 2-3                    | bytes 4-7                          | bytes 8-263                        |
|:---------|:----------------------------|:-----------------------------------|:-----------------------------------|
| 0x8006   | 260 | Write address | 256 bytes payload |

As it is a debug command, no boundary checks are performed. The place at which these 256 bytes are written should be previously erased.

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x8006   | 1 | 0x01 (indicates command success) |



0x8007: Reboot to Bootloader
----------------------------

| byte 0-1 | byte 2-3                    | bytes 4-X |
|:---------|:----------------------------|:----------|
| 0x8007   | 0 | Nothing |

Start main microcontroller bootloader. **No device answer**.



0x8008: Get 32 samples of accelerometer data
--------------------------------------------

| byte 0-1 | byte 2-3                    | bytes 4-X |
|:---------|:----------------------------|:----------|
| 0x8008   | 0 | Nothing |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x8008   | 192 | 32x (accX, accY, accZ - uint16_t) |



0x8009: Flash Aux MCU with bundle contents
------------------------------------------

| byte 0-1 | byte 2-3                    | bytes 4-X |
|:---------|:----------------------------|:----------|
| 0x8009   | 0 | Nothing |

Flash aux MCU with binary file included in the bundle. **No device answer**.



0x800A: Get platform info
-------------------------

| byte 0-1 | byte 2-3                    | bytes 4-X |
|:---------|:----------------------------|:----------|
| 0x800A   | 0 | Nothing |

Request platform info

Device Answer:

| Bytes | Description |
|:------|:------------|
| 0-1   | 0x800A |
| 2-3   | payload length (TBD) |
| 4-55  | see (aux_platform_spec_message)[here] |
| 56-67 | reserved |
| 67-68 | main MCU fw version, major |
| 69-70 | main MCU fw version, minor |

