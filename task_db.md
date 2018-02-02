**Time estimated**: very long  
**Skills**: C  
**Description**:  
The new Mini will implement an updated database model, including (among others) the following improvements:  
- categories for credentials 
- single credential for multiple services 
- native Unicode Plane 0 support 
- per credential finishing key press 
- previous password storage 
  
**Tasks**:  
- understand the previous and updated database models
- suggest improvements to the new model
- write down the differences between the old and new database model in order to make new code development easier, if you choose to base your code on the existing source code
- write down the low level code that will allow efficient communication with the external database flash (can be based on <a href="https://github.com/limpkin/mooltipass/blob/master/source_code/src/FLASH/flash_mem.c">our previous one</a>)
- write down the code for the new database block storing techniques: 264B blocks will be stored at the beginning of the flash while 528B blocks will be stored at its end in order to prevent DB fragmentation
- update (or rewrite) our <a href="https://github.com/limpkin/mooltipass/blob/master/source_code/src/NODEMGMT/node_mgmt.h">current code</a> in order to implement that new database model
- write down the documentation for the implemented written code
- ideally, create a flow chart explaining the list of of function calls that need to be performed in order to do basic tasks (create new credential, delete credential...) 

**Relevant links**:   
- <a href="https://docs.google.com/drawings/d/1oqjMz9LcSFaBZ23j24VP960ybm5YFqlvtHKBYQj-fFg">suggested database model</a>
- <a href="https://docs.google.com/drawings/d/1L_uYu-QqMRhFRf5I2WbTteFQDDU5MdqA4P6IraxyZ58">previous database model</a>
- <a href="https://github.com/limpkin/mooltipass/blob/master/source_code/src/NODEMGMT/node_mgmt.h#L143">previous database model code</a>
