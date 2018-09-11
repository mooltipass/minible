## [](#header-2) Status Message Sent by the Main MCU 
| Byte Numbers         | Description |
|:---------------------|:------------|
| 0-1 (Message type)   | 0x0003      |
| 2-3 (Payload Length) | 0           |
| 4-539 (Payload)      | do not care |
| 540-541 (Not used)   | 0x0000 |
| 542-543 (Reply request flag) | 0x0000 |


## [](#header-2) Status Message Sent by the Aux MCU 
  
| Byte Numbers         | Description |
|:---------------------|:------------|
| 0-1 (Message type)   | 0x0003      |
| 2-3 (Payload Length) | TBD         |
| 4-5 (Payload)        | aux MCU firmware version, major |
| 6-7 (Payload)        | aux MCU firmware version, minor |
| 8-11 (Payload)       | aux MCU device ID register (DSU->DID.reg) |
| 12-27 (Payload)      | aux MCU UID (registers 0x0080A00C 0x0080A040 0x0080A044 0x0080A048) |
| 28-29 (Payload)      | BluSDK library version (major) - running on MCU |
| 30-31 (Payload)      | BluSDK library version (minor) - running on MCU |
| 32-33 (Payload)      | BluSDK firmware version (major) - running on ATBTLC1000 |
| 34-35 (Payload)      | BluSDK firmware version (minor) - running on ATBTLC1000 |
| 36-37 (Payload)      | BluSDK firmware version (build no) - running on ATBTLC1000 |
| 38-41 (Payload)      | ATBTLC1000 RF version |
| 42-45 (Payload)      | ATBTLC1000 chip ID |
| 46-51 (Payload)      | ATBTLC1000 BD Address |
| X-539 (Payload)      | do not care |
| 540-541 (Payload #2) | 0x0000 |
| 542-543 (Valid flag) | 0x0000 |
