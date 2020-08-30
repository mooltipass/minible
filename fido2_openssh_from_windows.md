# Setting up FIDO2 authentication on Linux from Windows

## A failed approach using Windows WSL2
**WSL does not support USB devices connection out of the box.** This is left here for reference in case this changes in the future.
Please install the <a href="https://docs.microsoft.com/en-us/windows/wsl/install-win10">Windows Subsystem for Linux Installation Guide for Windows 10</a> to be able to use the latest openssh. An alternative installation guide can be found <a href="https://ubuntu.com/blog/ubuntu-on-wsl-2-is-generally-available">here</a>.  
In case you get "Error: 0x1bc", follow https://github.com/microsoft/WSL/issues/5651#issuecomment-663879171  
If you don't want to use the Windows store to install Ubuntu, follow these steps:
- Download ubuntu-20.04-server-cloudimg-amd64-wsl.rootfs.tar.gz on https://cloud-images.ubuntu.com/releases/focal/release/
- ```mkdir Ubuntu_2004```
- ```wsl --import Ubuntu-20.04 Ubuntu_2004 ubuntu-20.04-server-cloudimg-amd64-wsl.rootfs.tar.gz```
- ```wsl --list``` (Ubuntu-20.04 appears in the list)
- ```wsl -d Ubuntu-20.04``` (logged root)
- ```adduser <my_name>```
- ```usermod -aG sudo <my_name>```
- ```exit```
- ```wsl -d Ubuntu-20.04 -u <my_name>``` (logged <my_name>)
- ```cd```

You can then create a Windows shortcut targeting C:\WINDOWS\system32\cmd.exe /C "wsl -d Ubuntu-20.04 -u <my_name>".  
These instructions were found on https://github.com/MicrosoftDocs/WSL/issues/645#issuecomment-622363571
