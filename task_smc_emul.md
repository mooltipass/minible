**Title**: Smartcard Emulation  
**Time estimated**: short-medium  
**Skills**: C  
**Description**:  
Due to a lack of GPIO pins on the main microcontroller, the SWD CLK/IO are mapped to normally used smartcard signals. As a result, it is impossible to debug and use our smartcard at the same time. The goal of this task therefore is to implement smartcard emulation.  
**Tasks**:  
- Identify the low level routines involved in the <a href="https://github.com/mooltipass/minible/blob/master/source_code/main_mcu/src/SMARTCARD/smartcard_lowlevel.c">smartcard code</a>
- Create a wrapper for these functions
- Implement emulation functions, enabled using a #define
