This is a Python script that allows you to decrypt a Mooltipass backup by grabbing the AES key from your Mini BLE card using a standard USB smart card reader. This can be helpful if your Mooltipass device is ever lost/broken.

    WARNING: Using this tool effectively renders your Mooltipass device useless. Your AES key will be fetched from your card. Both your credential database and its decryption key will therefore be in your computer memory. If your computer is infected, all your logins & passwords can be decrypted without your knowledge. 


# Supported USB Smart Card Readers
ACR38U-I1 or ACR39U-U1

# Dependency Setup
## Windows
* Install Python 3.9.x from https://www.python.org (3.9.10 can be downloaded here: https://www.python.org/downloads/release/python-3910/)
* Install pyscard from https://sourceforge.net/projects/pyscard/files/pyscard/ (https://sourceforge.net/projects/pyscard/files/pyscard/pyscard%202.0.1/pyscard-2.0.1-cp39-cp39-win_amd64.whl/download - install the whl using `pip install pyscard-2.0.1-cp39-cp39-win_amd64.whl`, see https://github.com/LudovicRousseau/pyscard/blob/master/INSTALL.md)
* Install pycryptodome: `python -m pip install pycryptodome`

## Linux

* Install package dependencies, in case of Debian/Ubuntu: `apt install python3 python3-pip swig python3-tk libpcsclite-dev pcscd`
* Install python dependencies: `python3 -m pip install pyscard pycryptodome`

## OSX
These instructions are using brew for some dependencies, but they could also be built from source if desired.
* Install brew dependencies: `brew install python3 swig python-tk`
* Install python dependencies: `python3 -m pip install pyscard pycryptodome`

# Inserting Mini BLE Card Into Reader
The Mini BLE cards are smaller than a normal smart card, so they do not nicely fit in the reader. One option is to rip off the bottom of the card reader, so it's easy to see where the smart card needs to contact the reader. It should look like this when aligned properly.

![Reader Card Alignment](reader-card-alignment.jpeg?raw=true "Reader Card Alignment")

# Script Usage
To execute the script just run `smc_decode.py` with your USB smart card reader connected and the Mini BLE card inserted.

```
python3 smc_decode.py

Please insert your smartcard
connecting to ACS ACR39U ICC Reader

...

Correctly changed card type to AT88SC102

...

Correct AT88SC102 card inserted
Card Initialized by a Mooltipass Device
Number of PIN tries left: 4
User Card Detected
...

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  WARNING  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!                                                                                                       !
! Using this tool effectively renders your Mooltipass device useless.                                   !
! After accepting the following prompt, your AES key will be fetched from your card.                    !
! Both your credential database and its decryption key will therefore be in your computer memory.       !
! If your computer is infected, all your logins & passwords can be decrypted without your knowledge.    !
! Type "I understand" in the following prompt to proceed                                                !
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Text input: I understand

Please Enter Your PIN: xxxx

...

Correct PIN

...

AES key extracted: xxxx
Decrypting file with CPZ xxxx
Nonce: xxxx

Decrypting memory file contents...

... decrypted output ...

disconnecting from ACS ACR39U ICC Reader
disconnecting from ACS ACR39U ICC Reader
```

# Known bugs
If an incorrect pin is inserted, it's not possible to unlock the card anymore with the script. Simply put it back to your device, unlock it, then try again with the script.
