The goal of this page is to make a record of all observations regarding the Mini BLE breakout boards, as well as tracking down the modifications that were done on them.  
  
  
## [](#header-2)Hardware Bugs Found
1) To make sure that the system would be powered on on a wheel click, C12 value was increased from 1uF to 10uF. Unfortunately, it was discovered that the main MCU internal diode doesn't allow enough current to go through when the system is off. An extra diode therefore needs to be added between 3V3 and C12.  
2) 3V3 switching for the OLED stepup is too quick, causing a 3V3 drop and triggering the BOD. The power-up sequence therefore needs to be changed to first switch on the 1V5 then disable it, then enable the 3V3.  
3) U16 IO2 & IO3 (pin 3 & 7) are left unconnected, consuming current when in sleep. As a patch, pin 3 can be soldered with pin 4 (nWP not used when SRP not set) and pin 7 can be soldered with pin 8.  
  
   
## [](#header-2)Main MCU Deep Sleep Power Consumption
A firmware was made middle of August in order to check the power consumption when all ICs (except Aux MCU & ATBLTC) are in deep sleep.  
The firmware may be downloaded [here](ressources/2018-08-18-main-mcu-direct-sleep.hex), all switches ON except Aux MCU, ATBTLC, DISCHG & CHARGE, current measurement on the different test points.
When doing this test, make sure to remove the SWD adapter and to have applied patch 3) described above. 
Variation between boards is currently being investigated.  


| Breakout # | USB->3V | DATAFLASH | DBFLASH | 3V OLED | 3V SMC | 3V ACC | 3V MCU | 
|:-----------|:--------|:----------|:--------|:--------|:-------|:-------|:-------|
| 1  | 402uA |
| 2  | 26uA |
| 3  | 813uA |
| 4  | uA |
| 5  | uA |
| 6  | uA |
| 7  | uA |
| 8  | uA |
| 9  | uA |
| 10  | uA |
