**Title**: Secure Bootloader Implementation  
**Time estimated**: short - medium  
**Skills**: C  
**Description**:  
The new Mooltipass Mini includes a new microcontroller for which a new bootloader needs to be created. Currently, a basic bootloader, fetching the new firmware from external flash has been successfully coded and tested. Similarly to the current Mooltipass Mini, we now want to implement checking of the integrity and signature of the fetched new firmware.  
**Tasks**:  
- look for and implement cryptographic primitives for checking new firmware signatures (for example, the current mini uses fixed length CBC-MAC)
- upgrade the current code to handle firmware signing
