# Minible Aux MCU V2

miniBLE V2 port for aux mcu. This port is base on next hardware dev board and use an NRF52833 mcu to replace SAMD and BLE chip.

## Project structure

The main goal of this code is to port application code aux_mcu to this new hw
config but also trying to improve/clarify boundary between hw dedicated code and
pur app one. The main benefit of this is to simplify integration/management of
future hw evolution.

My proposition is:

 - keeping `LOGIC`, `COMM` and `fido2` folders as pur application code.
 - Describe necessary hw interface in `src/drivers` folder
 - Implementing hw interface in `src/ports/minibleV2` folder (for this v2)

## Zephyr config

 - zephyr sdk version: 1.0.1
 - zephyr version: 4.3.99

## How To build

 - install zephyr and zephyr sdk, instructions [here](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
 - zephyr version 4.4.x and sdk version 1.0.1
 - activate zephyr python env `source path/to/zephyr/.env/bin/activate.sh`
 - set environnement variables:
  + `ZEPHYR_BASE=path/to/zephyr/dir`
  + `ZEPHYR_TOOLCHAIN_VARIANT=path/to/sdk`
 - run `cmake -B build -DWITH_FREESTANDING=true` and `cmake --build build` freestanding is used by tinycbor
 - to flash run 

## Todo

- UART:
 + [x] bring up
 + [ ] dma
 + [ ] speed at  6 megabaud
 + [ ] hw flow control
- COMMS_RAW_HID
 + [ ] ???? nettoyage/refactoring
- BLE:
 + [ ] bring up
- [ ] hw restart 3x each power up
- [x] wheel click error ?

## Usefull informations

 - [zephyr dts syntax](https://docs.zephyrproject.org/latest/build/dts/intro-syntax-structure.html#id9)
