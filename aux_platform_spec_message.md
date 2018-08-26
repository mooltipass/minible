## [](#header-2) Status Message Sent by the Aux MCU 
  
| byte 0 - 1 | byte 2 - 3 |
|:-----------|:-----------|
| 0x0003     | TBD        |

**byte 4-5**: aux MCU firmware version, major  
**byte 6-7**: aux MCU firmware version, minor  
**byte 8-11**: aux MCU device ID register (DSU->DID.reg)  
**byte 12-27**: aux MCU UID (registers 0x0080A00C 0x0080A040 0x0080A044 0x0080A048)  
**byte 28-31**: ATBTLC1000 device id (register 0x4000B000), or 0s if no BLE IC   
**byte 32-35**: SDK version (2 MSBytes: major & minor, 4LSBytes: SDK build number)  
**byte 36-39**: BTLC1000 RF version  

| byte X - 539 | byte 540 - 541 | byte 542 - 543 |
|:-------------|:---------------|----------------|
| do not care  | 0x0000         | 0x0000         |
