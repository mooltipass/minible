## Communications Between Aux MCU Bootloader and Main MCU
1. Aux MCU Bootloader is executed during startup and it waits for "enter programming command" during a limited time of 349ms (0xFFFFFF/48000). After this time the Aux MCU will jump to application if a valid application is found (ie: jump address different than 0xFFFFFFFF).
 
2. The communication between Aux Mcu Bootloader and Main MCU is performed through a serial link, if the Main MCU does not send "Enter Programming" command within 349ms, the bootloader will jump to application and a reset should be performed in order to enter bootloader again.

3. Bootloader size needs to fit in 8kb size (or less). The application will start at a fixed address defined by the size of the bootloader.
  
## Message Structure and Serial Link Specs 
The communication from main mcu to bootloader are performed according:
[Aux Mcu <-> Serial Link Specification](aux_main_mcu_protocol.md)

The commands received by the bootloader are performed in the top of the protocol mentioned above with MessageType equal to __0x0002__.

The payload has a size of __536 bytes__, so the commands will have the following structure:

| Command | Data |
|:-:|:-:|
| Byte 0-1 | Byte 2-535 |

#### Enter Programming Command (0x0000)
| byte 0-1 | byte 2-3 | byte 4-7 | byte 8-11 |
|:-:|:-:|:-:|:-:|
| Command (2 bytes) | Reserved (2 bytes) | Image Length (4 bytes) | Image CRC (4 bytes) |

- __Command__: 0x0000
- __Reserved__: to preserve 4 byte alignment in the following fields.
- __Image Length__: Binary Image Length.
- __Image CRC__: CRC of binary image. (future usage)

#### Write Command (0x0001)
| byte 0-1 | byte 2-3 | byte 4-7 | byte 8-11 | 
|:-:|:-:|:-:|:-:|
| Command (2 bytes) | Size (2 bytes) | CRC (4 bytes) | Data (Size bytes) |

- __Command__: 0x0001
- __Size__: Number of data bytes. 256 bytes (1 row = 4xpages = 4x64B) or 512 bytes (2 rows). :warning: (use fixed 512 as start value) :warning:
- __CRC__: CRC of Data. (future usage)
- __Data__: Data to flash, it shall contain the application.
- On every Write command, erase row will be executed before, this will erase 4 pages (64B each page).
- The internal address to write will be incremented on each write command.
- After last write is performed, the bootloader will jump directly to application.

#### Aux Mcu Bootloader to Main Mcu (Response)
1. Echo as positive response, next command can be received
2. Full payload to 0xFF as negative response, main mcu shall reset the bootlooder and try again.

:warning: Main MCU shall wait for the answer of Aux MCU bootloader to transmit the next command :warning: