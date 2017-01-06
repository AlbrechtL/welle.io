dab-rpi
=====================
This repository contains the implementation of a simple DAB/DAB+ receiver. It is optimized for embedded systems like Raspberry Pi 2 and Raspberry Pi 3 but it runs on regulars PC as well.

The receiver supports terrestrial DAB and DAB+ reception with as input either the stream from an AIRSPY, a SDRplay, a dabstick (rtl_sdr), a rtl_tcp server or a (prerecorded) file, and it will output through the selected soundcard.

Table of contents
====

  * [Usage](#usage)
    * [GUIs](#guis)
    * [Command Line Parameters](#command-line-parameters)
    * [Settings INI-File](#settings-ini-file)
  * [Building](#building)
    * [General Information](#general-information)
    * [Ubuntu Linux 16.04 LTS](#ubuntu-linux-1604-lts)
    * [Windows 10](#windows-10)
    * [Raspberry Pi 2 and 3](#raspberry-pi-2-and-3)

Usage
====

GUIs
---
GUI | Description 
------ | ----------
GUI_1 | Is a GUI by using QT widgets.
GUI_2 | Is assumed that dab-rpi itself does not have a GUI. It is controlled remotely. More information can be found in the file [README.GUI_2](README.GUI_2]).
GUI_3 | Is a touch and high DPI display optimized GUI based in QT QML. But it runs also well on regular PCs.

Command Line Parameters
---

Parameter | Description | Valid for GUI
------ | ---------- | ----
i | TBD | GUI_1, GUI_2, GUI_3
S | Sync method. Default: 2 | GUI_1, GUI_2, GUI_3
D | Input device. Possible are: airspy, rtl_tcp, sdrplay, dabstick | GUI_2, GUI_3
M | DAB mode. Possible are: 1,2 or 4, Default: 1 | GUI_2, GUI_3
B | DAB band | GUI_2, GUI_3
I | rtl_tcp server IP address. Only valid for input rtl_tcp | GUI_2, GUI_3

Example usage:
  
  ```
# dab-rpi -D rtl_tcp -I 192.168.1.1
  ```
  
Settings INI-File
---
TBD 


Building
====================

General Information
---
The following libraries and their development files are needed:
* QT 4.8 and above for GUI_1 and GUI_2 
* QT 5.7 and above GUI_3
* portaudio 0.19
* FFTW3f
* libfaad
* libsndfile
* zlib
* librtlsdr (for dabstick)
* libusb
* libsamplerate

Two possibilities for building the software are there: the Qt qmake tools
or the CMake tools.

In the ".pro" file one may select (or deselect) input devices by uncommenting (commenting) the appropriate "CONFIG = XXX" lines.
A similar facility exists for the CMakeLists.txt file

Note that selecting a device requires installing the library and the development files.

Since an RPI is often run headless, an option is included to configure such that the PCM output is sent to a simple TCP server, listening at port 20040. Uncomment the following lines.

  ```
CONFIG += TCP_STREAMER will do here.
  ```
  
Note on MOT handling comment or uncomment the following lines for excluding or including a preliminary handling of slides in DAB

  ```
DEFINE	+= MOT_BASICS__
DEFINE	+= MOT_DATA__

  ```
  
Depending on the settings in the dab-rpi.pro file, the output is sent to the local "soundcard" or to a TCP port.
Add to following lines for having the output sent to port 20040.

  ```
CONFIG += tcp-streamer
  ```

Note that in that case there will be no sound output locally. I am using that feature to have the RPI on a location different from where I am normally sitting.
A local "listener" program is available to catch the data and transfer it to the soundcard. 

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
CONFIG		+= dabstick_osmo
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
DEFINES		+= MOT_BASICS__		# use at your own risk
DEFINES		+= MSC_DATA__		# use at your own risk
CONFIG		+= NO_SSE_SUPPORT 
#CONFIG		+= extio
#CONFIG		+= airspy
#CONFIG		+= airspy-exp
CONFIG		+= rtl_tcp
CONFIG		+= dabstick_osmo
#CONFIG		+= dabstick_new
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


