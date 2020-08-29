Welcome to the Mooltipass Mini BLE Firmware Repository!
=======================================================
<p align="center">
  <img src="https://github.com/mooltipass/minible/raw/master/_readme_assets/minible_front.jpg" alt="Mooltipass Mini BLE"/>
</p>
Here you will find the source code running on the Mooltipass Mini BLE auxiliary and main microcontrollers.  

What is the Mooltipass Project?
-------------------------------
The Mooltipass project is a complete ecosystem aimed at providing authentication solutions. It is composed of:  
- A <a href="https://github.com/mooltipass/minible_hw">physical device</a>, providing all security-related features  
- Multiple <a href="https://github.com/mooltipass/extension">browser extensions</a> (Chrome, Firefox, Edge, Opera) for easy credentials storage & recall  
- A <a href="https://github.com/mooltipass/moolticute">cross-plaform user interface</a>, for easy management of the physical device features and database  
- A <a href="https://github.com/mooltipass/moolticute">cross-platform software daemon</a>, serving as an interface between device and software clients  
- An <a href="https://github.com/raoulh/mc-agent">SSH agent</a>, providing password-less SSH authentication using a Mooltipass device  
- A <a href="https://github.com/oSquat/mooltipy">python library</a> to recall credentials stored on the Mooltipass
- A <a href="https://github.com/raoulh/mc-cli">command line tool written in go</a> to interact with the Mooltipass device

The Mooltipass Devices
----------------------
<p align="center">
  <img src="https://github.com/mooltipass/minible/raw/master/_readme_assets/ble_vaults_cards.png" alt="Mooltipass Mini BLE"/>
</p>
All Mooltipass devices (<a href="https://github.com/limpkin/mooltipass/tree/master/kicad/standard">Mooltipass Standard</a>, <a href="https://github.com/limpkin/mooltipass/tree/master/kicad/mini">Mooltipass Mini</a>, <a href="https://github.com/mooltipass/minible">Mooltipass Mini BLE</a>) are based on the same principle: each device contains one (or more) user database(s) AES-256 encrypted with a key stored on a PIN-locked smartcard. This not only allows multiple users to share one device but also one user to use multiple devices, as the user database can be safely exported and the smartcard securely cloned.  

The Mini BLE Architecture
-------------------------
<p align="center">
  <img src="https://github.com/mooltipass/minible/raw/master/_readme_assets/ble_architecture.png" alt="Mooltipass Mini BLE"/>
</p>
The firmwares in this repository are made for the device architecture shown above. The Mooltipass Mini BLE is composed of <b>two microcontrollers</b>: an <a href="https://github.com/mooltipass/minible/tree/master/source_code/aux_mcu">auxiliary one</a> dedicated to <b>USB and Bluetooth communications</b> and a <b>secure microcontroller</b> dedicated to running all security features. You may read about the rationale behind this choice <a href="https://mooltipass.github.io/minible/highlevel_overview">here</a>. The device microcontrollers communicate with each other using a <a href="https://mooltipass.github.io/minible/aux_main_mcu_protocol">high speed serial link</a>.
