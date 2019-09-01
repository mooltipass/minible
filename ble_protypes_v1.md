The goal of this page is to make a record of all observations regarding the Mini BLE first prototypes, as well as tracking down the modifications that were done on them.  
  
  
## [](#header-2)Hardware Bugs Found   
1) Already found in breakout boards: SMC detect MCU internal pull up consumes 74uA (around 45k), considerably increasing overall power consumption during sleep. An external pull-up should be used (1M) for the next iteration.  
   
## [](#header-2)Prototypes Locations   
  
| Breakout # | Location |
|:-----------|:----|
| 1          | @bernhard |
| 2          | @limpkin |
| 3          | @limpkin |
| 4          | @limpkin |
   
   
## [](#header-2)Platform sleep power consumptions
Deep sleep on main and aux MCU, ATBTLC1000 off, measured power consumption at 1V5 IN was 85uA, 88uA and 100uA. This increase compared to breakout boards is explained by switching to TPS61220 in variable output configuration.  
