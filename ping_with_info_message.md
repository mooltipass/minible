## Ping with Info Message

This message is sent by the main MCU to ping the aux MCU, as well as giving it information it may need.   

**From Main MCU:**  

| byte 0 - 1   | byte 2 - 3    | byte 4 - 5      | byte 6 - X                          |
|:-------------|:--------------|:----------------|-------------------------------------|
| 0x0007       | 2 (so far)    | main MCU status | DNC |
