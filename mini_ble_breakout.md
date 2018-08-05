The goal of this page is to make a record of all observations regarding the Mini BLE breakout boards, as well as tracking down the modifications that were done on them.  
  
  
## [](#header-2)Hardware Bugs Found
1) To make sure that the system would be powered on on a wheel click, C12 value was increased from 1uF to 10uF. Unfortunately, it was discovered that the main MCU internal diode doesn't allow enough current to go through when the system is off. An extra diode therefore needs to be added between 3V3 and C12.  
2) 3V3 switching for the OLED stepup is too quick, causing a 3V3 drop and triggering the BOD. The power-up sequence therefore needs to be changed to first switch on the 1V5 then disable it, then enable the 3V3.
  
   
## [](#header-2)Main MCU Deep Sleep Power Consumption
A firmware was made beginning of August in order to check the power consumption when all ICs (except Aux MCU & ATBLTC) are in deep sleep.  
The firmware may be downloaded [here](ressources/2018-08-05-main-mcu-direct-sleep.hex), all switches ON except Aux MCU, ATBTLC, DISCHG & CHARGE, current measurement on "USB->3V3" test points.  
When doing this test, make sure to remove the SWD adapter.  
Variation between boards is currently being investigated.  


| Breakout # | Current consumption |
|:-----|:--------|
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
