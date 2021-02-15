## [](#header-1) Mooltipass Protocol
This page explains the new Mooltipass messaging protocol and the different commands implemented in its firmware.
   
## [](#header-2) High Level Message Structure Overview

| byte 0-1           | byte 2-3       | bytes 4-X |
|:-------------------|:---------------|:----------|
| Command Identifier | Payload Length | Payload   |
   
Debug and test commands have identifiers above 0x8000.  
As with our previous devices, every non-debug / non-test message sent by the computer will receive an answer.  
Character size is 2 bytes in order to accomodate Unicode BMP.  
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

| byte 0-1 | byte 2-3         | bytes 4-7         | bytes 8-X         |
|:---------|:-----------------|:------------------|:------------------|
| 0x0002   | 4                | Unit's SN         | N/A               |

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
| 2->3   | 16     |
| 4->5   | Main MCU fw major |
| 6->7   | Main MCU fw minor |
| 8->9   | Aux MCU fw major |
| 10->11 | Aux MCU fw minor |
| 12->15 | Platform serial number |
| 16->17 | DB memory size |
| 18->19 | Bundle version |



0x0004: Set Current Date
------------------------

From the PC: 

| bytes  | value  |
|:-------|:-------|
| 0->1   | 0x0004 |
| 2->3   | 12 |
| 4->5   | year |
| 6->7   | month |
| 8->9   | day |
| 10->11 | hour |
| 12->13 | minute |
| 14->15 | second |

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



0x000B: Get Card Protected Zone
-------------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x000B   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4-11          |
|:---------|:----------------------------|:-------------------|
| 0x000B   | 1 (failure) or 8 | Card CPZ |



0x000C: Get Device Settings
---------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x000C   |        N/A   |

Device Answer:

| Bytes ID | Type | Description | Min | Max |         
|:---------|:-----|:------------|:----|:----|
| 0-1      | N/A  | 0x000C      | N/A | N/A |
| 2-3      | N/A  | WIP...      | N/A | N/A |
| 4 | N/A     | Reserved | N/A | N/A |
| 5 | bool    | Random PIN | 0 | N/A |
| 6 | uint8_t | User interaction timeout / 1024 | 7 | 25 |
| 7 | bool    | Animation during prompt | 0 | N/A |
| 8 | uint8_t | Device default language | 0 | WIP |
| 9 | uint8_t | Default char after login | 0 | 255 |
| 10 | uint8_t | Default char after pass | 0 | 255 |
| 11 | uint8_t | Delay between key presses | 0 | 255 |
| 12 | bool    | Boot Animation | 0 | N/A |
| 13 | uint8_t | Screen brightness | 0 | 255 |
| 14 | bool    | Device lock on USB disconnect | 0 | N/A |
| 15 | uint8_t | Knock detection sensitivity | 0 | 255 |

Notes:  

- char after login/press: for example, 0x09 for tab, 0x0A for enter


0x000D: Set Device Settings
---------------------------

From the PC: 

| byte 0-1 | byte 2-3     | byte 4-X |
|:---------|:-------------|:---------|
| 0x000D   |  WIP...      | aggregated settings (same as above) |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x000D   | 1 | 0x00 (failure) / 0x01 (success) |



0x000E: Reset Unknown Card
--------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x000E   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x000E   | 1 | 0x00 (failure) / 0x01 (success) |



0x000F: Get Number of Available Users
-------------------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x000F   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x000F   | 1 | Number of free users |



0x0010: Lock Device
-------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0010   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0010   | 1 | 0x00 (failure) / 0x01 (success) |



0x0011: Get Status
------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0011   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3 | byte 4 | byte 5 | byte 6-7 | byte 8 |
|:---------|:---------|:-------|:-------|:---------|:-------|
| 0x0011   | 5        | status byte, see below | battery pct | user security preferences | settings changed flag |

Status Byte:

| Bitmask  | Description |             
|:---------|:-----------|
| 0x01     | Smartcard presence |
| 0x02     | Not implemented |
| 0x04     | Smartcard unlocked |
| 0x08     | Unknown card inserted |
| 0x10     | Device in management mode |
| 0x20     | Device in no bundle or fw upload mode |

User Security Preferences (valid if byte 4 = 0x05):
See 0x0013

Settings changed flag: if different than 0, new **device** settings are available



0x0012: Check Credential
------------------------

From the PC: 

| bytes  | value  |
|:-------|:-------|
| 0->1   | 0x0012 |
| 2->3   | depends on message contents |
| 4->5   | index to the service name (0) |
| 6->7   | index to the login name |
| 8->9   | index to the password |
| 10->xxx | all above terminated fields concatenated |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0012   | 1 | 0x00 (fail), 0x01 (success), 0x02 (timeout protection) |



0x0013: Get User Settings
-------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0013   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4-5                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0013   | 0 or 2 | if payload length is 2, see below |

User Security Settings Bitmask:

| Bitmask  | Description |             
|:---------|:-----------|
| 0x01     | Login prompts on device |
| 0x02     | PIN required to enter MMM |
| 0x04     | Storage prompts during MMM |
| 0x08     | Advanced menu |
| 0x10     | Bluetooth enabled |
| 0x20     | Knock detection disabled |



0x0014: Get User Categories Strings
-----------------------------------

Strings need to be padded.
From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0014   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3 | byte 4-69 | byte 70-135 | byte 136-201| byte 202-268 |
|:---------|:---------|:----------|:----------|:----------|:----------|
| 0x0014   | 264 or 1 | String #1 | String #2 | String #3 | String #4 |



0x0015: Set User Categories Strings
-----------------------------------

From the PC: 

| byte 0-1 | byte 2-3 | byte 4-69 | byte 70-135 | byte 136-201| byte 202-268 |
|:---------|:---------|:----------|:----------|:----------|:----------|
| 0x0015   | 264      | String #1 | String #2 | String #3 | String #4 |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0015   | 1 | 0x00 (failure) / 0x01 (success) |



0x0016: Get User Language
-------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0016   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0016   | 1 | (if logged in) user language ID |



0x0017: Get Device Language
---------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0017   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0017   | 1 | device default language ID |



0x0018: Get User Keyboard Layout ID
-----------------------------------

From the PC: 

| byte 0-1 | byte 2-3     | byte 4    | byte 5-X     |
|:---------|:-------------|:----------|:-------------|
| 0x0018   |        1     | 0 for BLE, other for USB  |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0018   | 1 | BLE or USB keyboard layout ID |



0x0019: Get Number of Languages
-------------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0019   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4-5                        |
|:---------|:----------------------------|:--------------------------------|
| 0x0019   | 2 | number of languages supported |



0x001A: Get Number of Keyboard Layouts
--------------------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x001A   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4-5                        |
|:---------|:----------------------------|:--------------------------------|
| 0x001A   | 2 | number of layouts supported |



0x001B: Get Language Description
--------------------------------

From the PC: 

| byte 0-1 | byte 2-3     | byte 4    | byte 5-X     |
|:---------|:-------------|:----------|:-------------|
| 0x001B   |        1     | language ID  |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4-x                        |
|:---------|:----------------------------|:--------------------------------|
| 0x001B   | N/A | 0 terminated string |



0x001C: Get Layout Description
--------------------------------

From the PC: 

| byte 0-1 | byte 2-3     | byte 4    | byte 5-X     |
|:---------|:-------------|:----------|:-------------|
| 0x001C   |        1     | layout ID  |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4-x                        |
|:---------|:----------------------------|:--------------------------------|
| 0x001C   | N/A | 0 terminated string |



0x001D: Set User Keyboard Layout ID
-----------------------------------

From the PC: 

| byte 0-1 | byte 2-3     | byte 4    | byte 5 |
|:---------|:-------------|:----------|:-------|
| 0x001D   |        1     | 0 for BLE, other for USB  | layout ID |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x001D   | 1 | 0x00 (failure) / 0x01 (success) |



0x001E: Set User Language ID
----------------------------

From the PC: 

| byte 0-1 | byte 2-3     | byte 4    |
|:---------|:-------------|:----------|
| 0x001E   |        1     | Language ID |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x001E   | 1 | 0x00 (failure) / 0x01 (success) |



0x001F: Set Device Language ID
------------------------------

From the PC: 

| byte 0-1 | byte 2-3     | byte 4    |
|:---------|:-------------|:----------|
| 0x001F   |        1     | Language ID |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x001F   | 1 | 0x00 (failure) / 0x01 (success) |



0x0021: Create Data File
------------------------

From the PC: 

| byte 0-1 | byte 2-3                | byte 3-X                |
|:---------|:------------------------|:------------------------|
| 0x0021   | strlen(file_name)x2 + 2 | 0 terminated file name  |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0021   | 1 | 0x00 (failure) / 0x01 (success) |



0x0022: Store Data Into File
----------------------------

From the PC: 

| bytes    | value  |
|:---------|:-------|
| 0->1     | 0x0022 |
| 2->3     | 528 |
| 4->7     | 4B Set to 0 |
| 8->9     | Amount of bytes in this packet (from 0 to 512) |
| 10->265  | First (up to) 256B of data to store |
| 266->269 | 4B Set to 0 |
| 270->525 | Second (up to) 256B of data to store |
| 526->529 | Total file size |
| 530->531 | 0 to signal upcoming data, otherwise 1 to signal last packet |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0022   | 1 | 0x01 or 0x00 (success or fail) |



0x0023: Get Data File Chunk
---------------------------

From the PC: 

| byte 0-1 | byte 2-3                     | byte 3-X                |
|:---------|:-----------------------------|:------------------------|
| 0x0023   | 0 or strlen(file_name)x2 + 2 | nothing or 0 terminated file name  |

For a new data transfer, the file name must be specified. To get the next data chunk, set byte 2-3 to 0 (file name not specified).  
  
Device Answer:

| bytes    | value  |
|:---------|:-------|
| 0->1     | 0x0023 |
| 2->3     | depends |
| 4->5     | 0x01 or 0x00 (success or fail) |
| 6->7     | number of bytes in payload |
| 8->519   | data chunk |



0x0024: Inform Locked
---------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0024   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0010   | 1 | 0x00 (failure) / 0x01 (success) |

Inform the mooltipass device that the host is locked



0x0025: Inform Unlocked
-----------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0025   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0025   | 1 | 0x00 (failure) / 0x01 (success) |

Inform the mooltipass device that the host is unlocked



0x0026: Disable No Prompt
-------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0026   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0026   | 1 | 0x00 (failure) / 0x01 (success) |

Disable the no prompt feature on the current user profile.



0x0027: Store TOTP Credential
-------------------------

From the PC:

| bytes    | value                                 |
|:---------|:--------------------------------------|
| 0->1     | 0x0027                                |
| 2->3     | 448                                   |
| 4->255   | service name (null-terminated)        |
| 256->383 | login name (null-terminated)          |
| 384->431 | TOTP secret key                       |
| 432->432 | TOTP secret key length (max 64 bytes) |
| 433->433 | TOTP number of digits [6-8]           |
| 434->434 | TOTP time step [30 - 99]              |
| 435->435 | TOTP SHA version [0 - 2]              |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0027   | 1                           | 0x01 or 0x00 (success or fail)  |

Store new or updated TOTP credentials for a service/login combination.



0x0028: NiMH Recondition Start
------------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0028   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4-7                        |
|:---------|:----------------------------|:--------------------------------|
| 0x0028   | 4 | UINT32_MAX upon failure, otherwise discharge time |

Start the NiMH reconditioning procedure



0x0029: Start Bundle Upload
---------------------------

From the PC: 

| byte 0-1 | byte 2-3 | byte 4-19                |
|:---------|:---------|:------------------------|
| 0x0029   |       16 | upload password |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0029   | 1 | 0x00 (failure) / 0x01 (success) |

Start the bundle upload procedure (firmware flashing) and erases the data flash. Answer takes several seconds to arrive!



0x002A: Write 256B to flash
---------------------------

From the PC: 

| byte 0-1 | byte 2-3 | byte 4-7                 | byte 8-263               |
|:---------|:---------|:-------------------------|:-------------------------|
| 0x002A   |      260 | Memory address for write | Data to write |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x002A   | 1 | 0x00 (failure) / 0x01 (success) |

Write a chunk of data into the data flash.



0x002B: End Bundle Upload
-------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x002B   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x002B   | 1 | 0x00 (failure) / 0x01 (success) **or device reboot** |

End the bundle upload procedure. Device may reboot at the end of this message.



0x002C: Device Authentication Challenge
---------------------------------------

From the PC: 

| byte 0-1 | byte 2-3 | byte 4-23               |
|:---------|:---------|:-------------------------|
| 0x002C   |       20 | challenge (4bytes counter + signed challenge) |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4 - X                      |
|:---------|:----------------------------|:--------------------------------|
| 0x002C   | 1 or 16 | 0x00 (failure) or the challenge response |



0x002D: Get Diagnostics Data
----------------------------

From the PC: 

| byte 0-1 | byte 2-X |
|:---------|:---------|
| 0x002D   |     N/A  |

Device Answer:

| bytes    | value  |
|:---------|:-------|
| 0->1     | 0x002D |
| 2->3     | 16 |
| 4->7     | Nb ms screen ON MSBs |
| 8->11    | Nb ms screen ON LSBs |
| 12->15   | Nb 30mins on battery |
| 16->19   | Nb 30mins on USB |



0x002E: Inform of current service
---------------------------------

From the PC: 

| byte 0-1 | byte 2-3 | byte 4-X               |
|:---------|:---------|:-------------------------|
| 0x002E   | (service length + 1) x2 | 0 terminated string, each char as uint16_t |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x002E   | 1 | 0x01 (success) |

Inform device of currently visited service



0x002F: Modify Data File
------------------------

From the PC: 

| byte 0-1 | byte 2-3                | byte 3-X                |
|:---------|:------------------------|:------------------------|
| 0x002F   | strlen(file_name)x2 + 2 | 0 terminated file name  |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x002F   | 1 | 0x00 (failure) / 0x01 (success) |

Delete data file contents and allows 0x0022 packets



0x0030: Check Data File Presence
--------------------------------

From the PC: 

| byte 0-1 | byte 2-3                | byte 3-X                |
|:---------|:------------------------|:------------------------|
| 0x0030   | strlen(file_name)x2 + 2 | 0 terminated file name  |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0030   | 1 | 0x00 (failure) / 0x01 (file present) |



0x0031: Get Next Data Node Address & Name
-----------------------------------------

From the PC: 

| byte 0-1 | byte 2-3 | byte 4-5                |
|:---------|:---------|:------------------------|
| 0x0031   | 2        | Address to search from  |

Device Answer:

| byte 0-1 | byte 2-3                                      | byte 4-5       | byte 5-X  |
|:---------|:----------------------------------------------|:---------------|:----------|
| 0x0031   | 2 if nothing found, or 2 + strlen(name)x2 + 2 | Next node addr | Data name |



## [](#header-2) Memory Management Commands

If any of the commands below are sent when the device isn't in memory management mode, the reply will be a single 0x00 byte.


0x0100: Get Start Nodes Addresses
---------------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0100   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4-37                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0100   | 34 | 10 credential start address + 7 data start addresses |



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

| byte 0-1 | byte 2-3     | byte 4-5     |
|:---------|:-------------|:-------------|
| 0x0102   |        2     | Node address |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4-X                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0102   | 1 or 264 or 528 | 0x00 (failure) or memory node contents |



0x0103: Set Credential Change Number
------------------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | byte 4-7  |
|:---------|:----------------------------|:----------|
| 0x0103   | 4 | Credential change number |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0103   | 1 | 0x00 (failure) / 0x01 (success) |

NOT Tested


0x0104: Set Data Change Number
------------------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | byte 4-7  |
|:---------|:----------------------------|:----------|
| 0x0104   | 4 | Data change number |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0104   | 1 | 0x00 (failure) / 0x01 (success) |

NOT Tested



0x0105: Set Credential Start Parent
-----------------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | byte 4-5  | byte 6-7  |
|:---------|:----------------------------|:----------|:----------|
| 0x0105   | 4 | Data type ID | Start parent address |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0105   | 1 | 0x00 (failure) / 0x01 (success) |

NOT Tested



0x0106: Set Data Start Parent
-----------------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | byte 4-5  | byte 6-7  |
|:---------|:----------------------------|:----------|:----------|
| 0x0106   | 4 | Data type ID | Start parent address |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0106   | 1 | 0x00 (failure) / 0x01 (success) |

NOT Tested



0x0107: Set Start Nodes Addresses
---------------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | byte 4-37                       |
|:---------|:----------------------------|:--------------------------------|
| 0x0107   | 34 | 10 credential start address + 7 data start addresses |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0107   | 1 | 0x00 (failure) / 0x01 (success) |

NOT Tested



0x0108: Get Free Nodes Addresses
--------------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | byte 4-5 | byte 6-7 | byte 8-9 |
|:---------|:----------------------------|:---------|:---------|:---------|
| 0x0108   | 6 | Search start address  | # parent nodes | # child nodes |

Device Answer:

| Bytes ID | Description |             
|:---------|:-----------|
| 0-1      | 0x0108     |
| 2-3      | # addresses found x2     |
| 4-4+pnodesreqedx2 | available parent nodes addresses and 0 if not enough availability |
| 4+pnodesreqedx2-4+pnodesreqedx2+cnodesreqedx2      | available child nodes addresses and 0 if not enough availability     |

NOT Tested



0x0109: Get CTR Value
---------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x0109   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4-6                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0109   | 1 or 3 | 0x00 (failure) or CTR values |



0x010A: Set CTR Value
---------------------

From the PC: 

| byte 0-1 | byte 2-3                    | byte 4-6                       |
|:---------|:----------------------------|:--------------------------------|
| 0x010A   | 3 | CTR value |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x010A   | 1 | 0x00 (failure) / 0x01 (success) |

NOT Tested



0x010B: Set Favorite
--------------------

From the PC: 

| byte 0-1 | byte 2-3                    | byte 4-5 | byte 6-7 | byte 8-9 | byte 10-11 |
|:---------|:----------------------------|:---------|:---------|:---------|:---------|
| 0x010A   | 8 | category ID | favorite ID | parent addr | child addr |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x010A   | 1 | 0x00 (failure) / 0x01 (success) |

NOT Tested



0x010C: Get Favorite
--------------------

From the PC: 

| byte 0-1 | byte 2-3                    | byte 4-5 | byte 6-7 |
|:---------|:----------------------------|:---------|:---------|
| 0x010C   | 4 | category ID | favorite ID |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4-5 | byte 6-7 |
|:---------|:----------------------------|:---------|:---------|
| 0x010C   | 1 (failure) or 4 | parent addr | child addr |

NOT Tested



0x010F: Get Favorites
---------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x010F   |        N/A   |


Device Answer:

| byte 0-1 | byte 2-3                    | byte 4-203 |
|:---------|:----------------------------|:-----------|
| 0x010F   | 1 (failure) or 200 | all favorites concatenated |



0x010D: Write Node
------------------

From the PC: 

| byte 0-1 | byte 2-3                    | byte 4-5 | byte 6-XXXX |
|:---------|:----------------------------|:---------|:---------|
| 0x010D   | 2+(264 or 528) | write address | node |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x010D   | 1 | 0x00 (failure) / 0x01 (success) |



0x010E: Get Card Protected Zone & CTR Nonce
-------------------------------------------

From the PC: 

| byte 0-1 | byte 2-X     |
|:---------|:-------------|
| 0x010E   |        N/A   |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4-11          | byte 12-27         |
|:---------|:----------------------------|:-------------------|:-------------------|
| 0x010E   | 1 (failure) or 24 | Card CPZ | User AES CTR Nonce |



0x0110: Set Node Password
-------------------------

From the PC: 

| byte 0-1 | byte 2-3     | byte 3-X     |
|:---------|:-------------|:-------------|
| 0x0110   | Node address | Null terminated password |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x0110   | 1 | 0x00 (failure) / 0x01 (success) |

Command only available when simple mode is enabled, for a valid child node.


  
## [](#header-2) Mooltipass Debug and Test Commands

0x8000: Debug Message
-------------------

| byte 0-1 | byte 2-3                    | bytes 4-X                          |
|:---------|:----------------------------|:-----------------------------------|
| 0x8000   | strlen(debug_message)x2 + 2 | Debug Message + terminating 0x0000 |

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



0x800C: Set OLED parameters
---------------------------

From the PC: 

| byte 0-1 | byte 2-3 | byte 4            | byte 5 | byte 6 | byte 7 | byte 8 | byte 9 |
|:---------|:---------|:------------------|:-------|:-------|:-------|:-------|:-------|
| 0x800C   | 6        | contrast current  | vcomh  | vsegm  | precharge period  | discharge period  | vsl  |

Device Answer:

| byte 0-1 | byte 2-3                    | byte 4                          |
|:---------|:----------------------------|:--------------------------------|
| 0x800C   | 1 | 0x01 (indicates command success) |



0x800D: Get Battery Status
--------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | bytes 4-X                          |
|:---------|:----------------------------|:-----------------------------------|
| 0x800D   | 0 | Nothing |

Device Answer:

| bytes  | value  |
|:-------|:-------|
| 0->1   | 0x800D |
| 2->3   | 20     |
| 4->7   | Power source: 0 for USB, else battery |
| 8->11  | Charging status: 0 for false, else true |
| 12->13 | Main MCU ADC value (valid when battery powered) |
| 14->15 | Aux MCU charging state machine ID |
| 16->17 | Aux MCU ADC value (valid when charging) |
| 18->19 | Aux MCU charging current |
| 20->21 | Aux MCU step-down voltage |
| 22->23 | Aux MCU DAC DATA register value |



0x800E: Update Main & Aux MCU From Bundle
-----------------------------------------

From the PC: 

| byte 0-1 | byte 2-3                    | bytes 4-X                          |
|:---------|:----------------------------|:-----------------------------------|
| 0x800E   | 0 | Nothing |

Device Answer:

**None**. Device will disconnect from USB & Bluetooth and will reconnect just after.
