## [](#header-2) Main MCU to Aux MCU Commands    
These commands DO NOT get answers except for the ping (exact same packet replied)   
    
| Byte 0-1 | Byte 2-3 | Byte 4-5 | 6-541 | 542-544 | Description |  
|:---------|:---------|:---------|:------|:--------|:------------|   
| 0x0004   | 0x0002   | 0x0001   | DNC   | 0x0001  | Aux MCU Sleep |   
| 0x0004   | 0x0002   | 0x0002   | DNC   | 0x0001  | USB Attach |   
| 0x0004   | 0x0002   | 0x0003   | DNC   | 0x0001  | Ping (deprecated) |   
| 0x0004   | 0x0002   | 0x0004   | DNC   | 0x0001  | BLE Enable |   
| 0x0004   | 0x0002   | 0x0005   | DNC   | 0x0001  | NiMH Charge |   
| 0x0004   | 0x0002   | 0x0006   | DNC   | 0x0001  | No Comms Unav. |   
| 0x0004   | 0x0002   | 0x0007   | DNC   | 0x0001  | BLE Disable  |   
| 0x0004   | 0x0002   | 0x0008   | DNC   | 0x0001  | USB Detach |   
| 0x0004   | 0x0002   | 0x0009   | DNC   | 0x0001  | Func. Test Start |   
