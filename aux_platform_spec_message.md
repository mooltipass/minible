## [](#header-2) Status Message Sent by the Aux MCU 
  
| Byte Numbers         | Description |
|:---------------------|:------------|
| 0-1 (Message type)   | 0x0003      |
| 2-3 (Payload Length) | TBD         |
| 4-5 (Payload)        | aux MCU firmware version, major |
| 6-7 (Payload)        | aux MCU firmware version, minor |
| 8-11 (Payload)       | aux MCU device ID register (DSU->DID.reg) |
| 12-27 (Payload)      | aux MCU UID (registers 0x0080A00C 0x0080A040 0x0080A044 0x0080A048) |
| 28-31 (Payload)      | ATBTLC1000 device id (register 0x4000B000), or 0s if no BLE IC |
| 32-35 (Payload)      | SDK version (2 MSBytes: major & minor, 4LSBytes: SDK build number) |
| 36-39 (Payload)      | BTLC1000 RF version |
| X-539 (Payload)      | do not care |
| 540-541 (Payload #2) | 0x0000 |
| 542-543 (Valid flag) | 0x0000 |
