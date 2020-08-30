# Setting up FIDO2 authentication on Linux from Windows

## Prerequisites - OpenSSH
Version 8.3+ of OpenSSH is required for FIDO2 authentication to work on Windows. Unfortunately Windows typically uses an older version.   
As funny as it sounds, the simplest way to get it is to install <a href="https://git-scm.com/download/win">git for windows</a>.

## Prerequisites - OpenSSH SK WinHello
Download the latest **winhello.dll** from the <a href="https://github.com/tavrez/openssh-sk-winhello/releases">the OpenSSH SK WinHello project release page</a>.  
Copy it inside Git /usr/bin folder (C:\Program Files\Git\usr\bin by default).  

## Creating a private / public key
Launch **Git Bash**, then type ```ssh-keygen -w winhello.dll -t ecdsa-sk -f id_ecdsa_sk```. Approve the request on your device.  
In the same git Bash window, start the ssh agent by typing ```eval `ssh-agent -s` ```
Add the key to the agent by typing ```ssh-add id_ecdsa_sk```

## Adding the newly created key to the accepted list on your server
**Only if you don't have any public keys setup on your server**, an easy way to send your public keys to your server is to type ```scp id_ecdsa_sk.pub <<your_user_name>>@<<your_server_hostname_or_ip>>:~/.ssh/authorized_keys``` on the same git Bash window.

## Login into your server with your Mini BLE



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
