[welle.io](https://www.welle.io) [![Build Status](https://travis-ci.org/AlbrechtL/welle.io.svg?branch=master)](https://travis-ci.org/AlbrechtL/welle.io)
=====================
This repository contains the implementation of a SDR DAB/DAB+ receiver.  
Please see the project website https://www.welle.io for a user oriented documentation.

welle.io is fork from dab-rpi and sdr-j-dab which is now qt-dab https://github.com/JvanKatwijk/qt-dab.

Table of contents
====

  * [Usage](#usage)
  * [Supported Hardware](#supported-hardware)
  * [Building](#building)
    * [General Information](#general-information)
    * [Ubuntu Linux 16.04 LTS](#ubuntu-linux-1604-lts)
    * [Windows 10](#windows-10)
    * [macOS](#macos)
    * [Raspberry Pi 2 and 3](#raspberry-pi-2-and-3)
  * [Limitations](#limitations)
  * [Development](#development)

Usage
=====
The command line parameter are:

Parameter | Description
------ | ----------
h | Show help 
v | Show version 
D | Input device. Possible is: auto (default), airspy, rtl_tcp, rtl_sdr, rawfile, soapysdr
M | DAB mode. Possible is: 1,2,3 or 4, Default: 1 
I | rtl_tcp server IP address. Only valid for input rtl_tcp 
P | rtl_tcp server IP port. Only valid for input rtl_tcp
F | I/Q RAW file. Only valid for input rawfile.
B | I/Q RAW format. Possible is: u8 (unsigned int 8 bit, [qt-dab](https://github.com/JvanKatwijk/qt-dab) RAW files), s16le (signed int 16 bit little endian, [qt-dab](https://github.com/JvanKatwijk/qt-dab) SDR files), default: u8. Only valid for input rawfile.

Example usage:
  
  ```
# welle.io -D rtl_tcp -I 192.168.1.1 -P 1000
  ```
  ```
# welle.io -D rawfile -F test.sdr -B s16le
  ```

Supported Hardware
====================
The following SDR devices are supported
* airspy (http://airspy.com/)
* rtl_sdr (http://osmocom.org/projects/sdr/wiki/rtl-sdr)
* rtl_tcp (http://osmocom.org/projects/sdr/wiki/rtl-sdr#rtl_tcp)
* I/Q RAW file (https://www.welle.io/devices/rawfile)
* The LimeSDR through [SoapySDR](https://github.com/pothosware/SoapySDR) (Connect your antenna to `RX1_W`).

Building
====================

General Information
---
The following libraries and their development files are needed:
* QT 5.8 and above
* FFTW3f
* libfaad
* librtlsdr
* libusb

Ubuntu Linux 16.04 LTS
---
This sections shows how to compile welle.io on Ubuntu 16.04 LTS. 

1. Install QT 5.8 including the QT Charts module by using the the "Qt Online Installer for Linux" https://www.qt.io/download-open-source/

2. Install the following packages

  ```
# sudo apt install libfaad-dev libfftw3-dev librtlsdr-dev libusb-1.0-0-dev mesa-common-dev libglu1-mesa-dev libpulse-dev
  ```

3. (optional) Compile and install the airspy library. For details please see https://github.com/airspy/host/#how-to-build-the-host-software-on-linux

4. Clone welle.io

  ```
# git clone https://github.com/AlbrechtL/welle.io.git
  ```

5. Start QT Creator and open the project file "welle.io.pro" inside the folder "welle.io".
6. Build welle.io
7. Run welle.io and enjoy it

Windows 10
---
A compiled version can be found at https://github.com/AlbrechtL/welle.io/releases

This sections shows how to compile welle.io on Windows 10. Windows 7 should also be possible but is not tested. 

1. Install QT 5.8 including the QT Charts and mingw modules by using the the "Qt Online Installer for Windows" https://www.qt.io/download-open-source/
2. Clone welle.io https://github.com/AlbrechtL/welle.io.git e.g. by using [TortoiseGit](https://tortoisegit.org).
3. Clone the welle.io Windows libraries https://github.com/AlbrechtL/welle.io-win-libs.git.
4. Start QT Creator and open the project file "welle.io.pro" inside the folder "welle.io".
5. Build welle.io
6. Run welle.io and enjoy it

macOS
---
To build for macOS, you need to install the dependencies with macports first, assuming you have macports installed:

```
# sudo port install fftw-3-single faad2 rtl-sdr libusb
```

1. Install Qt 5.8 with Qt Creator directly from Qt website, not through macports.
2. Clone welle.io

  ```
# git clone https://github.com/AlbrechtL/welle.io.git
  ```

3. Open welle.io.pro with QT Creator.
4. Make sure in Qt Creator, "Projects, Build&Run, Run" that the checkbox "Add build library path to DYLD..." is off.
5. Build and run.

Raspberry Pi 2 and 3
---
To build and run welle.io on a Raspberry Pi 2 and 3 with GPU acceleration, please visit this repository: https://github.com/AlbrechtL/dab-rpi_raspbian_image (outdated)


Limitations
===
* CMake is not maintained (not working)

Development
===
You can join the welle.io development. Please visit the [wiki](https://github.com/AlbrechtL/welle.io/wiki) to find more information.
