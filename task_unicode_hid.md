**Title**: Unicode Plane 0 support implementation - Keyboard Presses  
**Time estimated**: long - very long  
**Skills**: C, Python  
**Description**:  
Keyboard presses are sent from a keyboard to a computer using each key unique identifier (https://github.com/limpkin/mooltipass/blob/master/source_code/src/USB/hid_defines.h) and NOT the ascii or unicode values of the characters printed on each key. The computer is therefore in charge of doing the key identifier to unicode character translation using the OS configured keyboard layout.
The current mini only supports manual typing of ascii characters. The main advantage of ascii characters is that they are 1 to 1 mapped with key identifiers, using upper key masks when needed. The mini therefore stores ascii character to key identifier lookup tables. These look up tables are generated using a python script which basically tells a mooltipass to type keys in order to bruteforce a given keyboard layout for the ascii keyspace. We want to migrate this system to Unicode Plane 0.  
**Tasks**:  
- play with our <a href="https://github.com/limpkin/mooltipass/tree/master/tools/keyboardLUTHidApi">python script</a> to understand how the mini outputs ascii characters through its HID interface
- modify said python script to accept unicode input, making sure it is correctly recognized as is 
- understand and identify the intricacies of western and eastern unicode characters input: for example, chinese characters are typed using multiple consecutive key presses
- make sure the current key identifiers inside the current mini are enough to generate all unicode characters for each language
- design a new unicode to key presses look up table storage format based on your findings
- modify our python script to allow specification of the unicode character space for which the unicode character to key identifier look up table needs to be generated
- modify our python script to generate binaries of these look up tables
- implement firmware support for these updated look up tables
