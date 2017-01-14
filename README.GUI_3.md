GUI_3
=====

GUI_3 is a fancy interface. Since the interface does not have buttons and selectors for mode, channel etc, some
parameters can be given as command line parameter on program start-up.
Most of the settings, passed on through the command line are maintained between program invocations, i.e. when you select
the airspy as input device, for Band IV and Mode IV, you only need to do it once.

Table of contents
====

  * [Usage](#usage)
  * [Building](#building)
    * [General Information](#general-information)
    * [Ubuntu Linux 16.04 LTS](#ubuntu-linux-1604-lts)
    * [Windows 10](#windows-10)
    * [Raspberry Pi 2 and 3](#raspberry-pi-2-and-3)

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
# dab-rpi -D rtl_tcp -I 192.168.1.1 -P 1000
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
This sections shows how to compile dab-rpi with GUI_3 on Ubuntu 16.04 LTS. 

1. Install QT 5.7 including the QT Charts module by using the the "Qt Online Installer for Linux" https://www.qt.io/download-open-source/

2. Install the following packages

  ```
# sudo apt install libfaad-dev libfftw3-dev portaudio19-dev librtlsdr-dev libusb-1.0-0-dev  libsndfile1-dev libsamplerate0-dev mesa-common-dev libglu1-mesa-dev zlib1g-dev git
  ```
3. Clone dab-rpi

  ```
# git clone https://github.com/JvanKatwijk/dab-rpi.git
  ```

4. Start QT Creator and open the project file "dab-rpi.pro" inside the folder "dab-rpi".
5. Edit "dab-rpi.pro" and adapt it to your needs. This example is tested with the following settings:

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

6. Build dab-rpi
7. Run dab-rpi and enjoy it

Windows 10
---
This sections shows how to compile dab-rpi with GUI_3 on Windows 10. Windows 7 should also be possible but is not tested. 

1. Install QT 5.7 including the QT Charts and mingw modules by using the the "Qt Online Installer for Windows" https://www.qt.io/download-open-source/
2. Clone dab-rpi https://github.com/JvanKatwijk/dab-rpi.git e.g. by using [TortoiseGit](https://tortoisegit.org).
3. Clone the dab-rpi Windows libraries https://github.com/AlbrechtL/dab-rpi_win_libs.git.
4. Start QT Creator and open the project file "dab-rpi.pro" inside the folder "dab-rpi".
5. Edit "dab-rpi.pro" and adapt it to your needs. This example is tested with the following settings:

  ```
win32 {
DESTDIR	= ../windows-bin-dab
# includes in mingw differ from the includes in fedora linux
#INCLUDEPATH += /usr/i686-w64-mingw32/sys-root/mingw/include
INCLUDEPATH += ../dab-rpi_win_libs/include
LIBS		+= -L/usr/i686-w64-mingw32/sys-root/mingw/lib
LIBS		+= -L../dab-rpi_win_libs/x86
LIBS		+= -lfftw3f-3
LIBS		+= -lportaudio_x86
LIBS		+= -llibsndfile-1
LIBS		+= -lole32
LIBS		+= -lwinpthread
LIBS		+= -lwinmm
LIBS 		+= -lstdc++
LIBS		+= -lws2_32
LIBS		+= -llibfaad
LIBS		+= -lusb-1.0
LIBS  += -llibsamplerate
LIBS  += -lzlib
DEFINES		+= MOT_BASICS__		# use at your own risk
DEFINES		+= MSC_DATA__		# use at your own risk
CONFIG		+= NO_SSE_SUPPORT 
#CONFIG		+= extio
#CONFIG		+= airspy
#CONFIG		+= airspy-exp
CONFIG		+= rtl_tcp
CONFIG		+= dabstick
#CONFIG		+= sdrplay
#CONFIG		+= tcp-streamer
#CONFIG		+= rtp-streamer
CONFIG		+= gui_3
}
  ```

6. Build dab-rpi
7. Run dab-rpi and enjoy it

Raspberry Pi 2 and 3
---
To build and run dap-rpi with GUI_3 on a Raspberry Pi 2 and 3 with GPU acceleration, please visit this repository: https://github.com/AlbrechtL/dab-rpi_raspbian_image


