welle.io
=====================
This repository contains the implementation of a simple DAB/DAB+ receiver. 
It is fork from dab-rpi (https://github.com/JvanKatwijk/dab-rpi) and from sdr-j-dab (https://github.com/JvanKatwijk/sdr-j-dab).

The receiver supports terrestrial DAB and DAB+ reception with as input the sample stream from a dabstick (rtl_sdr) or a rtl_tcp server


Table of contents
====

  * [Usage](#usage)
  * [Building](#building)
    * [General Information](#general-information)
    * [Ubuntu Linux 16.04 LTS](#ubuntu-linux-1604-lts)
    * [Windows 10](#windows-10)
    * [Raspberry Pi 2 and 3](#raspberry-pi-2-and-3)
  * [Limitations](#limitations)

Usage
=====
The command line parameter are:

Parameter | Description
------ | ---------- | ----
h | Show help 
v | Show version 
i | INI-file path. Do not use unless you know what you want.
S | Sync method. Do not use unless you know what you want.
D | Input device. Possible are: airspy, rtl_tcp, sdrplay, dabstick 
M | DAB mode. Possible are: 1,2 or 4, Default: 1 
B | DAB band. Default Band III
I | rtl_tcp server IP address. Only valid for input rtl_tcp 
P | rtl_tcp server IP port. Only valid for input rtl_tcp

Example usage:
  
  ```
# welle.io -D rtl_tcp -I 192.168.1.1 -P 1000
  ```
  
Building
====================

General Information
---
The following libraries and their development files are needed:
* QT 5.7 and above GUI_3
* portaudio 0.19
* FFTW3f
* libfaad
* libsndfile
* libsamplerate
* zlib
* librtlsdr (for dabstick),
* libusb

For building 
Use for building qmake.

In the ".pro" file one may select (or deselect) input devices by uncommenting (commenting) the appropriate "CONFIG = XXX" lines.
Note that selecting a device requires installing the library and the development files.

Ubuntu Linux 16.04 LTS
---
This sections shows how to compile welle.io with GUI_3 on Ubuntu 16.04 LTS. 

1. Install QT 5.7 including the QT Charts module by using the the "Qt Online Installer for Linux" https://www.qt.io/download-open-source/

2. Install the following packages

  ```
# sudo apt install libfaad-dev libfftw3-dev portaudio19-dev librtlsdr-dev libusb-1.0-0-dev  libsndfile1-dev libsamplerate0-dev mesa-common-dev libglu1-mesa-dev zlib1g-dev git
  ```
3. Clone welle.io

  ```
# git clone https://github.com/AlbrechtL/welle.io.git
  ```

4. Start QT Creator and open the project file "welle.io.pro" inside the folder "welle.io".
5. Edit "welle.io.pro" and adapt it to your needs. This example is tested with the following settings:

  ```
unix {
CONFIG		+= dabstick
#CONFIG		+= sdrplay-exp
#CONFIG		+= sdrplay
CONFIG		+= rtl_tcp
#CONFIG		+= airspy
#CONFIG		+= tcp-streamer		# use for remote listening
CONFIG		+= gui_3
DESTDIR		= ./linux-bin
INCLUDEPATH	+= /usr/local/include
LIBS		+= -lfftw3f  -lusb-1.0 -ldl  #
LIBS		+= -lportaudio
LIBS		+= -lz
LIBS		+= -lsndfile
LIBS		+= -lsamplerate
LIBS		+= -lfaad
}
  ```

6. Build welle.io
7. Run welle.io and enjoy it

Windows 10
---
This sections shows how to compile welle.io with GUI_3 on Windows 10. Windows 7 should also be possible but is not tested. 

1. Install QT 5.7 including the QT Charts and mingw modules by using the the "Qt Online Installer for Windows" https://www.qt.io/download-open-source/
2. Clone welle.io https://github.com/AlbrechtL/welle.io.git e.g. by using [TortoiseGit](https://tortoisegit.org).
3. Clone the welle.io Windows libraries https://github.com/AlbrechtL/welle.io-win-libs.git.
4. Start QT Creator and open the project file "welle.io.pro" inside the folder "welle.io".
6. Build welle.io
7. Run welle.io and enjoy it

Raspberry Pi 2 and 3
---
To build and run dap-rpi with GUI_3 on a Raspberry Pi 2 and 3 with GPU acceleration, please visit this repository: https://github.com/AlbrechtL/dab-rpi_raspbian_image

Limitation
===
* CMake is not tested
* DAB is not tested (only DAB+ is tested)
* sdrplay and airspy SDR devices are not tested 
