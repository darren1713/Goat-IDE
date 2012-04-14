## Go @ IDE is an editor dedicated to the embedded World

some features:

* Its editor (which is based on the wonderful library "scintilla" http://www.scintilla.org/), is lightweight and powerful.
* Has additional features such as "syntax highlighting", "code folding" and "code completion" based on the eLua API.
* Includes the capability to connect with the real hardware (through the serial or the USB port) obtaining the prompt of Elua inside it.
* It's able to send the .lua/.lc file on the Hardware (using the eLua xmodem protocol).
* Supports some terminal commands (standard ansi/vt100), so it's able to play eLua games like "hanoi" or "hungman" (using the eLua TERM capability).
* Supports the FLASH (on the target) programming through the DFU/Arduino boot loader.
* Supports easily the "libusb" through Lua scripts (experimental).
* It's easily extensible through Lua scripts.

## Goat currently includes the support for:

* eLua    http://www.eluaproject.net/
* Arduino http://www.arduino.cc (the installation of the "Arduino IDE 1.0" is required)

## Videos:
* http://www.youtube.com/watch?v=kUmujKemFjA
* http://www.youtube.com/watch?v=Z3g2Hu_FC3Q

## How to build and install on Linux:

* Be sure we have the package "libgtk+2.0" installed: 
* $ sudo apt-get install libgtk+2.0
* $ cd $HOME
* $ git clone git://github.com/nuccioraciti/Goat-IDE.git
* $ cd Goat-IDE
* $ cd scintilla/gtk
* $ make all
* $ cd $HOME/Goat-IDE/Goat
* $ make all
* $ sudo make uninstall (if isn't first installation)
* $ sudo make install

* In order to support the "dfu-programmer" the libusb1.0 library is required:
* $ sudo apt-get install libusb-1.0

## How to install on Windows:

* Download it from here: http://www.box.com/s/0hiq7bquvtrx7ktvprbn
* Install as a Windows application.
* Try it and please report problems and suggestions.
* ps.  Goat will install a directory "usb-driver" here:
*      "C:\Program Files (x86)\The Go @ IDE\usb-driver" which contains:
*      a) the Windows driver of the USB UART (...\dfu-driver directory).
*      b) the Windows driver of the DFU Programmer (...\uart-driver directory).

## How to build and install on Windows:

* Install the "Mingw/Msys" packages:
*    http://sourceforge.net/projects/mingw/files/latest/download?source=files
* Launch the setup "mingw-get-inst-xxxxxxxx.exe" and choice the "Download
  latest repository catalogues"
* Selecting following components: C and C++ compilers, "MSYS Basic System" and 
*    "Mingw Developer ToolKit".
* Donwnload Go@ sources from here: 
*     https://github.com/nuccioraciti/Goat-IDE/zipball/master
* Now unzip sources inside a directory for example inside the c:\temp directory
* Open the mingw shell and go on sources directory:
* $ cd /c/temp/nuccioraciti-Goat-IDE-xxxxxxx
* Build the scintilla library:
* $ cd /c/temp/nuccioraciti-Goat-IDE-xxxxxxx/scintilla/win32
* $ make
* Build Goat:
* $ cd /c/temp/nuccioraciti-Goat-IDE-xxxxxxx/Goat/win32
* $ make
* Copy runtime libraries:
* $ cp /mingw/bin/libgcc_s_dw2-1.dll /c/temp/nuccioraciti-Goat-IDE-xxxxxxx/Goat/bin/
* $ cp /mingw/bin/libstdc++-6.dll /c/temp/nuccioraciti-Goat-IDE-xxxxxxx/Goat/bin/
* Launch the Go@ editor:
* Use the windows explorer, localise Goat.exe and double click on it.


## What it requires on the eLua side...

* It requires some optionali modules included inside the eLua core:

* #define BUILD_XMODEM
* #define BUILD_SHELL
* #define BUILD_TERM
and if it is avaliable on our hardware:
* #define BUILD_MMCFS

## Quick start.
* If your Linux is "Ubuntu" you can find "Goat" on menu: applications for development
* Then after you have connected one eLua board, please set the serial port name on Goat, using the menu: "Options" -> "Open Global Options File", un-commenting the right choice for you system. 
* After each change to the settings, you need to exit and reopen Goat again...
* Now you should have the eLua prompt on the "eLua's Console" window.
* Well, try to use the menu: "Open Example" and load some elua program (eg. hello.lua)
* The last step, press "F5" or menu "Tools" -> "Send & Run on Ram" will send the "hello.lua" directly on memory of eLua board and will run it.

(*) just a note, some eLua examples ("hanoi" and "hungman") requiring a special configuration of the "eLua Console" window, so don't forget to select the menu: "Tools" -> "Mode Terminal".

Enjoy with the "Go @ IDE" and thank you for testing, improving and leaving your feedback for it.

raciti.nuccio(AT)gmail.com 

