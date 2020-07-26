## [](#header-2)Developing Without a Device  
With our device emulator, it is now possible for Mooltipass enthusiasts to contribute to our ecosystem without actually owning a device or SWD programmer!

## [](#header-2)Windows  
Working with our emulator on Windows involves the use of QT Creator.

### [](#header-3)Step 1: Downloading the Mini BLE Repository  
Have a look at [this great GitHub tutorial](https://docs.github.com/en/github/getting-started-with-github/fork-a-repo).  
If you prefer a command line tool, you may want to check out [git for windows](https://gitforwindows.org/).  
Once you have forked our repository, **do not forget to initialize all its submodules** by typing `git submodule update --init --recursive` if you are using a command line based tool for git.

### [](#header-3)Step 2: Installing QT Creator  
To compile our emulator, download and install QT Creator from [here](https://www.qt.io/download-thank-you?hsLang=en).  
You will need to create a QT account, and when prompted for installation options, select **MSVC 2019 64-bit and MinGW 8.1.0 64-bit** under "QT 5.15.0" before moving on with the installation process:  
   
![](https://github.com/mooltipass/minible/blob/gh-pages/images/emulator_tuto/0_qt_options.png?raw=true)

### [](#header-3)Step 3: Compiling the Emulator  
Start QT Creator, click on "File" then "Open File or Project" and open **minible_emu** located in source_code/main_mcu/. Then click "Configure Project":  

![](https://github.com/mooltipass/minible/blob/gh-pages/images/emulator_tuto/1_project_configuration.PNG?raw=true)
  
The finally, click the green arrow!  

![](https://github.com/mooltipass/minible/blob/gh-pages/images/emulator_tuto/2_compilation_start.PNG?raw=true)

### [](#header-3)Step 4 : Providing Missing Bundle  

![](https://github.com/mooltipass/minible/blob/gh-pages/images/emulator_tuto/3_missing_bundle.PNG?raw=true)

If all went well during the compilation, you should see the two emulator windows above appear, with the emulator asking for a bundle file. Simply copy the "miniblebundle.img" file located in source_code/main_mcu/emu_assets folder to Qt Creator build's folder (something like build-minible_emu-Desktop_QT_5_15...).  
Close both emulator windows, start the compilation step again and ta-da!

![](https://github.com/mooltipass/minible/blob/gh-pages/images/emulator_tuto/4_emulator_working.PNG?raw=true)


### [](#header-3)Step 5 : Emulator Control & Mooltipass Team Ground Rules  

You may use the keyboard arrow keys to control the emulator, or use your mouse with its scroll wheel.  
We can't wait to collaborate with you! Do not forget to have a look at the [Mooltipass Team Ground Rules](https://mooltipass.github.io/minible/coding_rules)  
  
  
## [](#header-2)Ubuntu  
Getting up and running on Ubuntu is slightly faster than on Windows!  

### [](#header-3)Step 1: Downloading the Mini BLE Repository  
First install git:  
`sudo apt-get install git`  
Then clone our repository:  
`git clone https://github.com/mooltipass/minible`  
Initialize all its submodules:  
`cd minible`  
`git submodule update --init --recursive`  

### [](#header-3)Step 2: Compiling the Emulator  
Install all prerequisites:  
`sudo apt-get install build-essential qt5-default`  
Compile the emulator:  
`cd source_code/main_mcu/`  
`make -f Makefile.emu`  
Then finally launch it:  
`./build/minible`  

### [](#header-3)Step 3 : Providing Missing Bundle  

![](https://github.com/mooltipass/minible/blob/gh-pages/images/emulator_tuto/3_missing_bundle_ubuntu.PNG?raw=true)  
  
If all went well during the compilation, you should see the two emulator windows above appear, with the emulator asking for a bundle file.    
Close both emulator windows and type the following command:  
`sudo cp emu_assets/miniblebundle.img /usr/share/misc/`  
  
![](https://github.com/mooltipass/minible/blob/gh-pages/images/emulator_tuto/4_emulator_working_ubuntu.PNG?raw=true)


### [](#header-3)Step 4 : Emulator Control & Mooltipass Team Ground Rules  

You may use the keyboard arrow keys to control the emulator, or use your mouse with its scroll wheel.  
We can't wait to collaborate with you! Do not forget to have a look at the [Mooltipass Team Ground Rules](https://mooltipass.github.io/minible/coding_rules)  

