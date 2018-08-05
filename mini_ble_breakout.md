The goal of this page is to make a record of all observations regarding the Mini BLE breakout boards, as well as tracking down the modifications that were done on the different boards.

## [](#header-2)Main MCU deep sleep power consumtion
A firmware was made beginning of August in order to check the power consumption when all ICs (except aux MCU & ATBLTC) are in deep sleep.  
The firmware may be downloaded [here](ressources/2018-08-05-main-mcu-direct-sleep.hex), all switches ON except Aux MCU, ATBTLC, DISCHG & CHARGE, current measurement on "USB->3V3".  
When doing this test, make sure to remove the SWD adapter.  


| Breakout # | Current consumption |
|:-----|:--------|
| 1  | 565uA (see history) |
| 2  | uA   |
| 3  | uA   |
| 4  | uA   |
| 5  | uA   |
| 6  | uA   |
| 7  | uA   |
| 8  | uA   |
| 9  | uA   |
| 10  | uA   |
