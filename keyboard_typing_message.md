## Keyboard Typing Message

This message is sent by the main MCU to tell the aux MCU to type a list of symbols.

**From Main MCU:**  

| byte 0 - 1   | byte 2 - 3        | byte 4 - 5    | byte 6 - 7            | byte 8 - X                          |
|:-------------|:------------------|:--------------|:----------------------|-------------------------------------|
| 0x0008       | Payload Length    | HID interface | Delay between presses | 0 terminated list of 16bits symbols |

HID interface: 0 for USB, 1 for BLE  
Delay between presses: delay in ms between each HID report send

**Reply from Aux MCU:**  
  
| byte 0 - 1   | byte 2 - 3        | byte 4 - 5                                    | byte 6 - 559      |
|:-------------|:------------------|:----------------------------------------------|:------------------|
| 0x00008      | 2                 | 0 if typing failure, something else otherwise | DNC               |

**16bits symbol format:**  

| byte 0 | Description       |
|:-------|:------------------|
| 0xFF   | ignore symbol |
| 0x00   | byte 1 contains key & modifier  |
| 0x80   | byte 1 contains key & modifier, dead key bitmask |
| else   | (first) key + modifier |
  
| byte 1 | Description       |
|:-------|:------------------|
| 0x00   | (second) key + modifier  |

**Key + modifier format**  
  
| bit 7         | bit 6         | bit 5 to 0       |
|:--------------|:--------------|:-----------------|
| shift bitmask | altgr bitmask | HID code to send |

**HID code**

HID code to send. 0x03 is remapped to 0x64  
After parsing multiple keyboard mappings we found the following key codes to not be used: 0x00 0x01 0x02 0x03 0x28 0x29 0x3A 0x3B 0x3C 0x3D 0x3E 0x3F, allowing us to implement the 16bit symbol structure described above.
