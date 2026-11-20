# Minible Aux MCU V2

miniBLE V2 port for aux mcu. This port is base on next hardware dev board and use an NRF52833 mcu to replace SAMD and BLE chip.

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
