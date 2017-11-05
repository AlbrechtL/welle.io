[welle.io](https://www.welle.io)
=====================
- Linux (Travis): [![Travis Build Status](https://travis-ci.org/AlbrechtL/welle.io.svg?branch=master)](https://travis-ci.org/AlbrechtL/welle.io)
- Windows (AppVeyor): [![AppVeyor Build status](https://ci.appveyor.com/api/projects/status/yipsu95pb4ecdofe?svg=true)](https://ci.appveyor.com/project/AlbrechtL/welle-io)

This repository contains the implementation of a SDR DAB/DAB+ receiver.  
Please see the project website https://www.welle.io for a user oriented documentation. You can also use the forum https://forum.welle.io to get in contact with us.

welle.io is fork from dab-rpi and sdr-j-dab which is now qt-dab https://github.com/JvanKatwijk/qt-dab.

Table of contents
====

  * [Download](#download)
  * [Usage](#usage)
  * [Supported Hardware](#supported-hardware)
  * [Building](#building)
    * [General Information](#general-information)
    * [Ubuntu Linux 16.04 LTS](#ubuntu-linux-1604-lts)
    * [Windows 10](#windows-10)
    * [macOS](#macos)
    * [CMake instead of Qt Creator (Windows, Linux, macOS)](#cmake-instead-of-qt-creator-windows-linux-macos)
    * [Android](#android)
    * [Raspberry Pi 2 and 3](#raspberry-pi-2-and-3)
  * [Limitations](#limitations)
  * [Development](#development)

Download
========
At the moment there is no stable release available. But the releases are tested and working in the most cases. If you discovered an issue use the [forum](https://forum.welle.io/) or open a new [issue](https://github.com/AlbrechtL/welle.io/issues) please.
 * ### [welle.io for Windows, Linux and Android (APK)](http://github.com/AlbrechtL/welle.io/releases)
 * ### [welle.io for Android at Google Play](https://play.google.com/store/apps/details?id=io.welle.welle)

welle.io is under heavy development. You can also try the latest developer builds. But PLEASE BE WARNED the builds are automatically created and untested.
 * [welle.io nightly builds](https://bintray.com/albrechtl/welle.io/welle.io_nightly#files)

To use it on macOS or on a Raspberry Pi you have to compile welle.io direct from the sources.

Usage
=====
The command line parameter are:

Parameter | Description
------ | ----------
h | Show help 
v | Show version 
L | GUI language e.g. de_DE
D | Input device. Possible is: auto (default), airspy, rtl_tcp, rtl_sdr, rawfile, soapysdr
--sdr-driver-args | The value depends on the SDR driver and is directly passed to it (currently only SoapySDR::Device::make(args)). A typical value for SoapySDR is a string like driver=remote,remote=127.0.0.1,remote:driver=rtlsdr,rtl=0
--sdr-antenna | The value depends on the SDR Hardware, typical values are TX/RX, RX2. Just query it with SoapySDRUtil --probe=driver=uhd
--sdr-clock-source | The value depends on the SDR Hardware, typical values are internal, external, gpsdo. Just query it with SoapySDRUtil --probe=driver=uhd
M | DAB mode. Possible is: 1,2,3 or 4, Default: 1 
I | rtl_tcp server IP address. Only valid for input rtl_tcp 
P | rtl_tcp server IP port. Only valid for input rtl_tcp
F | I/Q RAW file. Only valid for input rawfile.
B | I/Q RAW format. Possible is:<ul><li>u8 (unsigned int 8 bit, [qt-dab](https://github.com/JvanKatwijk/qt-dab) RAW files)</li><li>s8 (signed 8 bit, [ODR](https://www.welle.io/devices/rawfile#odr-dabmod) files)</li><li>s16le (signed int 16 bit little endian, [qt-dab](https://github.com/JvanKatwijk/qt-dab) SDR files)</li><li>s16be (signed int 16 bit big endian, [qt-dab](https://github.com/JvanKatwijk/qt-dab) SDR files)</li><li>Default: u8. Only valid for input rawfile.</li></ul>

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
* All SDR-devices that are supported by SoapySDR, gr-osmosdr and uhd. These are too many devices to list them all. To see if your SDR is supported, have a look at the lists at [SoapySDR](https://github.com/pothosware/SoapySDR/wiki) and [SoapyOsmo](https://github.com/pothosware/SoapyOsmo/wiki).
    * Devices supported by gr-osmosdr are supported via [SoapyOsmo](https://github.com/pothosware/SoapyOsmo/wiki)
    * Devices supported by uhd are supported via [SoapyUHD](https://github.com/pothosware/SoapyUHD/wiki)
    * One limitation is of course that the SDR devices must be tunable to the DAB+ frequencies.

SoapySDR Notes
---

### LimeSDR

Connect the Antenna to the RX1_W port and start welle-io with the options -D soapysdr --sdr-antenna LNAW. SoapySDRUtil --probe=driver=lime may show other possible options.

### USRP

Start welle-io with -D soapysdr --sdr-driver-args driver=uhd --sdr-antenna <antenna> --sdr-clock-source <clock source>. To list possible values for antenna and clock source use the command "SoapySDRUtil --probe=driver=uhd".

Building
====================

General Information
---
The following libraries and their development files are needed:
* QT 5.9 (don't use QT 5.8 because of [this](https://github.com/AlbrechtL/welle.io/issues/35) bug)
* FFTW3f
* libfaad
* librtlsdr
* libusb

Ubuntu Linux 16.04 LTS
---
This sections shows how to compile welle.io on Ubuntu 16.04 LTS. 

1. Install QT 5.9 including the QT Charts module by using the the "Qt Online Installer for Linux" https://www.qt.io/download-open-source/

2. Install the following packages

  ```
# sudo apt install libfaad-dev libfftw3-dev librtlsdr-dev libusb-1.0-0-dev mesa-common-dev libglu1-mesa-dev libpulse-dev
  ```

3. (optional) Compile and install the airspy library. For details please see https://github.com/airspy/host/#how-to-build-the-host-software-on-linux.  
If you don't install the airspy library you have to disable the airspy for the welle.io build. Open welle.io.pro and outcomment the following line.
  ```
#CONFIG += airspy
  ```

4. Clone welle.io

  ```
# git clone https://github.com/AlbrechtL/welle.io.git
  ```

5. Start QT Creator and open the project file "welle.io.pro" inside the folder "welle.io".
6. Build welle.io
7. Run welle.io and enjoy it

Windows 10
---
A compiled version can be found at the [release page](https://github.com/AlbrechtL/welle.io/releases)

This sections shows how to compile welle.io on Windows 10. Windows 7 should also be possible but is not tested. 

1. Install QT 5.9 including the QT Charts and mingw modules by using the the "Qt Online Installer for Windows" https://www.qt.io/download-open-source/
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

1. Install Qt 5.9 with Qt Creator directly from Qt website, not through macports.
2. Clone welle.io

  ```
# git clone https://github.com/AlbrechtL/welle.io.git
  ```

3. Open welle.io.pro with QT Creator.
4. Make sure in Qt Creator, "Projects, Build&Run, Run" that the checkbox "Add build library path to DYLD..." is off.
5. Build and run.

CMake instead of Qt Creator (Windows, Linux, macOS)
---

As an alternative to Qt Creator, CMake can be used for building welle.io after installing dependencies and cloning the repository:

1. Create a build directory inside the repository and change into it

  ```
# mkdir build
# cd build
  ```

2. Run CMake. To enable support for RTL-SDR add the flag `-DRTLSDR=1` (requires librtlsdr) and for SoapySDR add `-DSOAPYSDR=1` (requires SoapySDR compiled with support for each desired hardware like the AirSpy or HackRF)

  ```
# cmake ..
  ```

  or to enable support for both RTL-SDR and Soapy-SDR:

  ```
# cmake .. -DRTLSDR=1 -DSOAPYSDR=1
  ```

3. Run make (or use the created project file depending on the selected generator)

  ```
# make
  ```

4. Run welle.io and enjoy it

Android
---
A compiled version APK can be found at at the [Google Play store](https://play.google.com/store/apps/details?id=io.welle.welle) or at the [release page](https://github.com/AlbrechtL/welle.io/releases).  
welle.io uses the ["RTL2832U driver"](https://play.google.com/store/apps/details?id=marto.rtl_tcp_andro)([sources](https://github.com/martinmarinov/rtl_tcp_andro-)) from Martin Marinov.

This sections shows how to compile welle.io for Android.

1. Install QT 5.9 for Android including the QT Charts and QT Remote Objects modules by using the the "Qt Online Installer for Windows" https://www.qt.io/download-open-source/
2. Follow the side https://doc.qt.io/qt-5/androidgs.html to install the Android build enviroment
3. Clone welle.io https://github.com/AlbrechtL/welle.io.git

  ```
# git clone https://github.com/AlbrechtL/welle.io.git
  ```
  
4. Start QT Creator and open the project file "welle.io.pro" inside the folder "welle.io".
5. Build welle.io
6. Run welle.io and enjoy it

Raspberry Pi 2 and 3
---
To build and run welle.io on a Raspberry Pi 2 and 3 with GPU acceleration, please visit this repository: https://github.com/AlbrechtL/dab-rpi_raspbian_image (outdated)


Limitations
===
* Windows 8 and older are not offically supported

Development
===
You can join the welle.io development. Please visit the [wiki](https://github.com/AlbrechtL/welle.io/wiki) to find more information.
