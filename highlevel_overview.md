## [](#header-2)The New Mooltipass Device
![](https://github.com/mooltipass/minible/blob/gh-pages/images/minible_highlevel.png?raw=true)
Contrary to all our previous Mooltipass devices, our new platform uses 2 main microcontrollers: a secure a non-secure one.  
This design decision was made for several reasons:  
- reduce the attack surface by only having a serial link exposed to the outside word
- the selected cheap Bluetooth transceiver requires proprietary libraries
- possibility to use any USB library on the non secure microcontroller
- lack of GPIO pins for the selected cheap microcontroller  
  
### [](#header-3)Security Constraints
- The secure MCU can disable the non-secure MCU to guarantee the boot sequence
- The secure MCU can disable the Bluetooth transceiver using an enable signal
- The secure MCU may communicate with previous Mooltipass smartcards
- We aim to not use any libraries on the secure MCU
- There is no restrictions on the non-secure MCU

### [](#header-3)Power Supplies
- A hardware design choice needs to be made: onboard battery or module?
- The device may be powered through an AAA battery or through USB
- Complete shut-off functionality is implemented
- Less than 100uA sleep current is targetted

### [](#header-3)Flash Memories
- The dataflash contains graphics elements as well as firmware updates
- The DB flash contains users logins and passwords
