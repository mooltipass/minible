## [](#header-2) Aux MCU to Main MCU Events    
These events DO NOT get answers.  
    
| Byte 0-1 | Byte 2-3 | Byte 4-5 | 6-559 | Description |  
|:---------|:---------|:---------|:------|:------------|   
| 0x0005   | 0x0002   | 0x0001   | DNC   | BLE Enabled |   
| 0x0005   | 0x0002   | 0x0002   | DNC   | BLE Disabled |   
| 0x0005   | 0x0002   | 0x0003   | DNC   | DBG TX Sweep Done |   
| 0x0005   | 0x0003   | 0x0004   | N/A   | Functional Test Done |   
| 0x0005   | 0x0002   | 0x0005   | DNC   | USB Enumerated |   
| 0x0005   | 0x0002   | 0x0006   | DNC   | NiMH Charge Done |   
| 0x0005   | 0x0002   | 0x0007   | DNC   | NiMH Charge Fail |   
| 0x0005   | 0x0002   | 0x0008   | DNC   | Sleep Command Received |   
| 0x0005   | 0x0002   | 0x0009   | DNC   | I'm Here! |   

Functional test 1 byte payload:  
0: Success
1: BLE failure
2: NiMH charging failure
