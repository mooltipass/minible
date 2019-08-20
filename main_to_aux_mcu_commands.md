## [](#header-2) Main MCU to Aux MCU Commands    
These commands DO NOT get answers except for the ping (exact same packet replied)   
    
| Byte 0-1 | Byte 2-3 | Byte 4-5 | 6-559 | Description |  
|:---------|:---------|:---------|:------|:------------|   
| 0x0004   | 0x0002   | 0x0001   | DNC   | Aux MCU Sleep |   
| 0x0004   | 0x0002   | 0x0002   | DNC   | USB Attach |   
| 0x0004   | 0x0002   | 0x0003   | DNC   | Ping (deprecated) |   
| 0x0004   | 0x0002   | 0x0004   | DNC   | BLE Enable |   
| 0x0004   | 0x0002   | 0x0005   | DNC   | NiMH Charge Start |   
| 0x0004   | 0x0002   | 0x0006   | DNC   | No Comms Unavailable |   
| 0x0004   | 0x0002   | 0x0007   | DNC   | BLE Disable  |   
| 0x0004   | 0x0002   | 0x0008   | DNC   | USB Detach |   
| 0x0004   | 0x0002   | 0x0009   | DNC   | Functional Test Start |   
