 # [](#header-2)Developing Without a Device on Windows
With our device emulator, it is now possible for Mooltipass enthusiasts to contribute to our ecosystem without actually owning a device or SWD programmer!

### [](#header-3)Downloading the Mini BLE Repository
Have a look at [this great GitHub tutorial](https://docs.github.com/en/github/getting-started-with-github/fork-a-repo).  
If you prefer a command line tool, you may want to check out [git for windows](https://gitforwindows.org/).  
Once you have forked our repository, **do not forget to initialize all its submodules** by typing `git submodule update --init --recursive` if you are using a command line based tool for git.

### [](#header-3)Installing QT Creator
To compile our emulator, download and install QT Creator from [here](https://www.qt.io/download-thank-you?hsLang=en).  
You will need to create a QT account, and when prompted for installation options, select **MSVC 2019 64-bit and MinGW 8.1.0 64-bit** under "QT 5.15.0" before moving on with the installation process:  
   
![](https://github.com/mooltipass/minible/blob/gh-pages/images/emulator_tuto/0_qt_options.png?raw=true)

### [](#header-3)Compiling the Emulator
Start QT Creator, click on "File" then "Open File or Project" and open **minible_emu** located in source_code/main_mcu/ .  
Then click "Configure Project":  

![](https://github.com/mooltipass/minible/blob/gh-pages/images/emulator_tuto/1_project_configuration.PNG?raw=true)
  
The finally, click the green arrow!  

![](https://github.com/mooltipass/minible/blob/gh-pages/images/emulator_tuto/2_compilation_start.PNG?raw=true)
