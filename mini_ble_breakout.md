The goal of this page is to make a record of all observations regarding the Mini BLE breakout boards, as well as tracking down the modifications that were done on them.  
  
  
## [](#header-2)Hardware Bugs Found   
1) To make sure that the system would be powered on by a wheel click, C12 value was increased from 1uF to 10uF. Unfortunately, it was discovered that the main MCU internal diode doesn't allow enough current to go through when the system is off. As a result, the main MCU needs to charge the capacitor when powering off. Another patch may be to add an extra diode Voled stepup in and C12.  
2) 3V3 switching for the OLED stepup is too quick, causing a 3V3 drop and triggering the BOD. The power-up sequence therefore needs to be changed to first switch on the 1V5 then disable it, then enable the 3V3.  
3) U16 IO2 & IO3 (pin 3 & 7) are left unconnected, consuming current when in sleep. As a patch, pin 3 can be soldered with pin 4 (nWP not used when SRP not set) and pin 7 can be soldered with pin 8.  
4) R8 & R7 resistors switched.  
5) 1V2 to 3V3 and USB to 3V3 are connected together through ideal diodes. As a result, both 1V2 and USB 5V will be used at the same time for 3V3 generation, preventing us from accurately measuring the current used for charging the NiMH battery.  
6) SMC detect MCU internal pull up consumes 74uA (around 45k), considerably increasing overall power consumption during sleep.  
  
    
## [](#header-2)Board Specific Bugs Found   
1) Board #3: PMOS Q3 not working. Patch: pin 2 & 3 shorted for always on functionality.   
   
    
## [](#header-2)Prototypes Locations   
  
| Breakout # | Location |
|:-----------|:----|
| 1          | @oliver |
| 2          | @limpkin |
| 3          | @raoulh |
| 4          | uA |
| 5          | @amro |
| 6          | uA |
| 7          | @limpkin, no aux MCU |
| 8          | @Dennis |
| 9          | @peter & amro |
   
   
## [](#header-2)Main MCU Deep Sleep Power Consumption
A firmware was made middle of August in order to check the power consumption when all ICs (except Aux MCU & ATBLTC) are in deep sleep.  
The firmware may be downloaded [here](ressources/2018-08-18-main-mcu-direct-sleep.hex), all switches ON except Aux MCU, ATBTLC, DISCHG & CHARGE, current measurement on the different test points and power through USB.  
When doing this test, make sure to remove the SWD adapter and to have applied patch 3) described above.  
  
  
| Breakout # | USB->3V | DATAFLASH | DBFLASH | 3V OLED | 3V SMC | 3V ACC | 3V MCU | 
|:-----------|:--------|:----------|:--------|:--------|:-------|:-------|:-------|
| 1          | 13uA    | 1uA       | 1uA     | 1uA     | 0uA    | 6uA    | 7uA    |
| 2          | 13uA    | 1uA       | 0uA     | 1uA     | 1uA    | 6uA    | 8uA    |
| 3          | 13uA    | 1uA       | 0uA     | 1uA     | 1uA    | 5uA    | 6uA    |
| 4          | 13uA    | 0uA       | 0uA     | 0uA     | 0uA    | 5uA    | 7uA    |
| 5          | 17uA    | 0uA       | 0uA     | 0uA     | 0uA    | 5uA    | 13uA    |
| 6          | 12uA    | 0uA       | 0uA     | 0uA     | 0uA    | 5uA    | 6uA    |
| 7          | 13uA    | 0uA       | 0uA     | 0uA     | 0uA    | 5uA    | 6uA    |
| 8          | 13uA    | 0uA       | 0uA     | 0uA     | 0uA    | 5uA    | 6uA    |
| 9          | 13uA    | 0uA       | 0uA     | 0uA     | 0uA    | 5uA    | 6uA    |
| 10         | 13uA    | 0uA       | 0uA     | 0uA     | 0uA    | 5uA    | 6uA    |
  
  
Current measurement at the 1V2 input, DISCHG switch ON and 1V2 applied:  
   
| Breakout # | 1V2 IN |
|:-----------|:----|
| 1          | 93uA |
| 2          | 92uA |
| 3          | 96uA |
| 4          | uA |
| 5          | uA |
| 6          | uA |
| 7          | uA |
| 8          | uA |
| 9          | uA |
| 10         | uA |
