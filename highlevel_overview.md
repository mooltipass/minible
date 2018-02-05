## [](#header-2)The New Mooltipass Device
![](https://github.com/mooltipass/minible/blob/gh-pages/images/minible_highlevel.png?raw=true)
Contrary to all our previous Mooltipass devices, our new platform uses 2 main microcontrollers: a MCU considered as secure and a standard one. In practice, they are MCUs from the same family.    
This design decision was made for several reasons:  
- reduce the attack surface by only having a serial link exposed to the outside word
- the selected cheap Bluetooth transceiver requires proprietary libraries
- the possibility to use any USB library on the non-secure microcontroller
- the lack of GPIO pins for the selected secure microcontroller  
  
### [](#header-3)Security Constraints
- The secure MCU can disable the non-secure MCU to guarantee the boot sequence
- The secure MCU can disable the Bluetooth transceiver through an enable signal
- The secure MCU may communicate with previous Mooltipass smartcards
- We aim to not use any libraries on the secure MCU
- There are no restrictions on the non-secure MCU

### [](#header-3)Power Supplies
- A hardware design choice needs to be made: onboard battery or battery module?
- The device may be powered through an AAA battery or through USB
- Complete power off functionality is implemented
- Less than 100uA sleep current is targetted

### [](#header-3)Flash Memories
- The dataflash contains graphic elements as well as firmware updates
- The DB flash contains users' logins and passwords
