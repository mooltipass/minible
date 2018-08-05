The goal of this page is to make a record of all observations regarding the Mini BLE breakout boards, as well as tracking down the modifications that were done on them.  
  
   
## [](#header-2)Main MCU deep sleep power consumption
A firmware was made beginning of August in order to check the power consumption when all ICs (except Aux MCU & ATBLTC) are in deep sleep.  
The firmware may be downloaded [here](ressources/2018-08-05-main-mcu-direct-sleep.hex), all switches ON except Aux MCU, ATBTLC, DISCHG & CHARGE, current measurement on "USB->3V3" test points.  
When doing this test, make sure to remove the SWD adapter.  


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
