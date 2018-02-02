**Title**: Unicode Plane 0 support implementation - Fonts  
**Time estimated**: short-medium  
**Skills**: C, Python  
**Description**:
From <a href="https://msdn.microsoft.com/en-us/magazine/mt763237.aspx">https://msdn.microsoft.com/en-us/magazine/mt763237.aspx</a>:   
"Unicode defines a concept of plane as a continuous group of 65,536 (2^16) code points. The first plane is identified as plane 0 or Basic Multilingual Plane (BMP). Characters for almost all modern languages and many symbols are located in the BMP, and all these BMP characters are represented in UTF-16 using a single 16-bit code unit.”
An external flash is dedicated to storing the fonts for each supported language. Each unicode character is uniquely identified using a single 16-bit code unit.
We want to generate font data packages for each language."  
At the moment, our font data package only supports one contiguous segment of unicode characters. We would therefore like to implement support for 2 continuous segments in order to optimize our data storage technique in case a font data package was to contain ascii characters (located at the beginning of the unicode address space) and a foreign language whose characters are located at the end of the unicode address space.  
**Tasks**: 
- update the font data package generation scripts to include 2 continous unicode characters segments
- identify and generate font data packages for each language using our existing tools
Relevant links:
- <a href="https://msdn.microsoft.com/en-us/magazine/mt763237.aspx">information about unicode</a>
- <a href="https://unicode-table.com/en/#cyrillic">unicode character table</a>
