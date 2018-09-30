[welle.io](https://www.welle.io)
=====================
- Linux (Travis): [![Travis Build Status](https://travis-ci.org/AlbrechtL/welle.io.svg?branch=master)](https://travis-ci.org/AlbrechtL/welle.io)
- Windows (AppVeyor): [![AppVeyor Build status](https://ci.appveyor.com/api/projects/status/yipsu95pb4ecdofe?svg=true)](https://ci.appveyor.com/project/AlbrechtL/welle-io)

# WARNING: This is the next branch and heavy under development! It may not to compile or work!

This repository contains the implementation of an SDR DAB/DAB+ receiver.  
Please see the project website https://www.welle.io for a user oriented documentation. 

welle.io is a fork from dab-rpi and sdr-j-dab which is now qt-dab https://github.com/JvanKatwijk/qt-dab.

Table of contents
====

  * [Download](#download)
  * [Usage](#usage)
  * [Supported Hardware](#supported-hardware)
  * [Building](#building)
    * [General Information](#general-information)
    * [Ubuntu Linux 16.04 LTS and 18.04 LTS](#ubuntu-linux-1604-lts-and-1804-lts)
    * [Windows 10](#windows-10)
    * [macOS](#macos)
    * [CMake instead of Qt Creator (Windows, Linux, macOS)](#cmake-instead-of-qt-creator-windows-linux-macos)
    * [Android](#android)
    * [Raspberry Pi 2 and 3](#raspberry-pi-2-and-3)
  * [welle-cli](#welle-cli)
    * [Usage](#usage-of-welle-cli)
    * [Backend options](#backend-options)
    * [Examples](#examples)
  * [Limitations](#limitations)
  * [Development](#development)

Download
========
Stable release can be found here:
 * ### [welle.io for Windows, Linux and Android (APK)](http://github.com/AlbrechtL/welle.io/releases)
 * ### [welle.io for Android at Google Play](https://play.google.com/store/apps/details?id=io.welle.welle)

If you discovered an issue please open a new [issue](https://github.com/AlbrechtL/welle.io/issues).

welle.io is under heavy development. You can also try the latest developer builds. But PLEASE BE WARNED the builds are automatically created and untested.
 * [welle.io nightly builds](https://bintray.com/albrechtl/welle.io/welle.io_nightly#files)

To use it on macOS or on a Raspberry Pi you have to compile welle.io directly from the sources. See below for more information.

Usage
=====
The command-line parameters are:

Parameter | Description
------ | ----------
-h, --help | Show help 
-v, --version | Show version 
-d, --device | Input device. Possible values are: auto (default), airspy, rtl_tcp, rtl_sdr, rawfile, soapysdr
--rtl_tcp-address | rtl_tcp server IP address. Only valid for input rtl_tcp 
--rtl_tcp-port | rtl_tcp server IP port. Only valid for input rtl_tcp
--raw-file | I/Q RAW file. Only valid for input rawfile.
--raw-format | I/Q RAW format. Possible is:<ul><li>u8 (unsigned int 8 bit, [qt-dab](https://github.com/JvanKatwijk/qt-dab) RAW files)</li><li>s8 (signed 8 bit, [ODR](https://www.welle.io/devices/rawfile#odr-dabmod) files)</li><li>s16le (signed int 16 bit little endian, [qt-dab](https://github.com/JvanKatwijk/qt-dab) SDR files)</li><li>s16be (signed int 16 bit big endian, [qt-dab](https://github.com/JvanKatwijk/qt-dab) SDR files)</li><li>cf32 (complex float, native endianness)</li><li>Default: u8. Only valid for input rawfile.</li></ul>
--soapysdr-driver-args | The value depends on the SoapySDR driver and is directly passed to it (currently only SoapySDR::Device::make(args)). A typical value for SoapySDR is a string like driver=remote,remote=127.0.0.1,remote:driver=rtlsdr,rtl=0
--soapysdr-antenna | The value depends on the SoapySDR Hardware, typical values are TX/RX, RX2. Just query it with SoapySDRUtil --probe=driver=uhd
--soapysdr-clock-source | The value depends on the SoapySDR Hardware, typical values are internal, external, gpsdo. Just query it with SoapySDRUtil --probe=driver=uhd
--dab-mode | DAB mode. Possible values are: 1,2,3, or 4, Default: 1 
--dump-file | Records DAB frames (*.mp2) or DAB+ superframes with RS coding (*.dab). This file can be used to analyse X-PAD data with XPADxpert (https://www.basicmaster.de/xpadxpert).
--log-file | Log file name. Redirects all log output texts to a file.
--language | Sets the GUI language according to the ISO country codes e.g. de_DE


Example usage:
  
  ```
# welle.io -d rtl_tcp --rtl_tcp-address 192.168.1.1 --rtl_tcp-port 1000
  ```
  ```
# welle.io -d rawfile --raw-file test.sdr --raw-format s16le
  ```

Supported Hardware
====================
The following SDR devices are supported
* Airspy R2 and Airspy Mini (http://airspy.com/)
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

Connect the antenna to the RX1_W port and start welle-io with the options `-d soapysdr --soapysdr-antenna LNAW`. `SoapySDRUtil --probe=driver=lime` may show other possible options.

### USRP

Start welle-io with `-d soapysdr --soapysdr-driver-args driver=uhd --soapysdr-antenna <antenna> --soapysdr-clock-source <clock source>`. To list possible values for antenna and clock source use the command `SoapySDRUtil --probe=driver=uhd`.

Building
====================

General Information
---
The following libraries and their development files are needed:
* Qt 5.10 (Qt 5.9 and below is not supported)
* FFTW3f
* libfaad
* librtlsdr
* libusb

Ubuntu Linux 16.04 LTS and 18.04 LTS
---
This section shows how to compile welle.io on Ubuntu 16.04 LTS and Ubuntu 18.04 LTS. 

1. Install Qt 5.10 including the Qt Charts module by using the the "Qt Online Installer for Linux" https://www.qt.io/download-open-source/

2. Install the following packages

  ```
# sudo apt install libfaad-dev mpg123.h libmpg123-dev libfftw3-dev librtlsdr-dev libusb-1.0-0-dev mesa-common-dev libglu1-mesa-dev libpulse-dev
  ```

3. (optional) Compile and install the Airspy library. For details please see https://github.com/airspy/host/#how-to-build-the-host-software-on-linux.  
If you don't install the Airspy library you have to disable the Airspy for the welle.io build. Open `welle.io.pro` and outcomment the following line:

```
#CONFIG += airspy
```

4. Clone welle.io

  ```
# git clone https://github.com/AlbrechtL/welle.io.git
  ```

5. Start Qt Creator and open the project file `welle.io.pro` inside the folder "welle.io".
6. Build welle.io
7. Run welle.io and enjoy it

Windows 10
---
A compiled version can be found at the [release page](https://github.com/AlbrechtL/welle.io/releases)

This sections shows how to compile welle.io on Windows 10. Windows 7 should also be possible but is not tested. 

1. Install Qt 5.10 including the Qt Charts and mingw modules by using the the "Qt Online Installer for Windows" https://www.qt.io/download-open-source/
2. Clone welle.io https://github.com/AlbrechtL/welle.io.git e.g. by using [TortoiseGit](https://tortoisegit.org).
3. Clone the welle.io Windows libraries https://github.com/AlbrechtL/welle.io-win-libs.git.
4. Start Qt Creator and open the project file `welle.io.pro` inside the folder "welle.io".
5. Build welle.io
6. Run welle.io and enjoy it

macOS
---

To build for macOS, you have have several options: Either you install everything incl. dependencies manually (not covered here and not recommended) or use Homebrew or MacPorts.

### Homebrew

Assuming that you have [Homebrew](https://brew.sh/) installed, execute the following steps:

1. Use the welle.io repository as a "tap" (alternative package repository):

```
# brew tap AlbrechtL/welle_io https://github.com/AlbrechtL/welle.io
```

2. Install welle.io (and dependencies):

```
# brew install AlbrechtL/welle_io/welle.io
```

### MacPorts

You need to install the dependencies with MacPorts first, assuming you have [MacPorts](https://www.macports.org/) installed:

```
# sudo port install fftw-3-single faad2 rtl-sdr libusb
```

1. Install Qt 5.10 with Qt Creator directly from Qt website, not through MacPorts.
2. Clone welle.io

  ```
# git clone https://github.com/AlbrechtL/welle.io.git
  ```

3. Open welle.io.pro with Qt Creator.
4. Make sure in Qt Creator, "Projects, Build&Run, Run" that the checkbox "Add build library path to DYLD..." is off.
5. Build and run.

CMake instead of Qt Creator (Windows, Linux, macOS)
---

As an alternative to Qt Creator, CMake can be used for building welle.io after installing dependencies and cloning the repository. On Linux, you can also use CMake to build [**welle-cli**](#welle-cli), the command-line backend testing tool that does not require Qt.

1. Create a build directory inside the repository and change into it

  ```
# mkdir build
# cd build
  ```

2. Run CMake. To enable support for RTL-SDR add the flag `-DRTLSDR=1` (requires librtlsdr) and for SoapySDR add `-DSOAPYSDR=1` (requires SoapySDR compiled with support for each desired hardware, e.g. UHD for Ettus USRP, LimeSDR, Airspy or HackRF). By default, CMake will build both welle-io and welle-cli. Use `-DBUILD_WELLE_IO=OFF` or `-DBUILD_WELLE_CLI=OFF` to compile only the one you need.

  ```
# cmake ..
  ```

  or to enable support for both RTL-SDR and Soapy-SDR:

  ```
# cmake .. -DRTLSDR=1 -DSOAPYSDR=1
  ```

  If you wish to use KISS FFT instead of FFTW (e.g. to compare performance), use `-DKISS_FFT=ON`.

3. Run make (or use the created project file depending on the selected generator)

  ```
# make
  ```

4. Install it (as super-user)

  ```
# make install
  ```

5. Run welle.io and enjoy it

Android
---
A compiled version of welle.io (APK file) can be found at at the [Google Play store](https://play.google.com/store/apps/details?id=io.welle.welle) or at the [release page](https://github.com/AlbrechtL/welle.io/releases).

welle.io uses the "RTL2832U driver" from Martin Marinov, to be found at the [Google play store](https://play.google.com/store/apps/details?id=marto.rtl_tcp_andro) or at [F-droid](https://f-droid.org/packages/marto.rtl_tcp_andro/). Also see ([sources](https://github.com/martinmarinov/rtl_tcp_andro-) or [APK file](https://github.com/martinmarinov/rtl_tcp_andro-/blob/master/app/app-release.apk)). Please note that a recent version of this driver is needed (v3.06 or above), otherwise welle.io will not find your stick.

This sections shows how to compile welle.io for Android.

1. Install Qt 5.10 for Android including the Qt Charts and Qt Remote Objects modules by using the the "Qt Online Installer for Windows" https://www.qt.io/download-open-source/
2. Follow the side https://doc.qt.io/qt-5/androidgs.html to install the Android build enviroment
3. Clone welle.io https://github.com/AlbrechtL/welle.io.git

  ```
# git clone https://github.com/AlbrechtL/welle.io.git
  ```
  
4. Start Qt Creator and open the project file `welle.io.pro` inside the folder "welle.io".
5. Build welle.io
6. Run welle.io and enjoy it

Raspberry Pi 2 and 3
---
To build and run welle.io on a Raspberry Pi 2 and 3 with GPU acceleration, please read [this guide](https://github.com/AlbrechtL/welle.io/blob/master/RASPBERRY-PI.md).

welle-cli
==

If you compile welle-io with [`cmake`](#cmake-instead-of-qt-creator-windows-linux-macos) you will also get an executable called **welle-cli** which stands for welle-io **c**ommand **l**ine **i**nterface. 

Usage of welle-cli 
---

Receive using RTLSDR, and play with ALSA:

    welle-cli -c channel -p programme

Read an IQ file and play with ALSA: (IQ file format is u8, unless the file ends with FORMAT.iq)

    welle-cli -f file -p programme

Use -D to dump FIC and all programmes to files:
 
    welle-cli -c channel -D 

Use -w to enable webserver, decode a programme on demand:
    
    welle-cli -c channel -w port

Use -Dw to enable webserver, decode all programmes:
    
    welle-cli -c channel -Dw port

Use `-C 1 -w` to enable webserver, decode programmes one by one in a carousel.
Use `-C N -w` to enable webserver, decode programmes N by N in a carousel.
This is useful if your machine cannot decode all programmes simultaneously, but you still want to get an overview of the ensemble.
By default welle-cli will switch every 10 seconds.
With the `-P` option, welle-cli will switch once DLS and a slide were decoded, staying at most for 80 seconds on a given programme.

    welle-cli -c channel -C 1 -w port
    welle-cli -c channel -PC 1 -w port
    
Example: `welle-cli -c 12A -C 1 -w 7979` enables the webserver on channel 12A, please then go to http://localhost:7979/ where you can observe all necessary details for every service ID in the ensemble, see the slideshows, stream the audio (by clicking on the Play-Button), check spectrum, constellation, TII information and CIR peak diagramme.

Backend options
---

`-u` disable coarse corrector, for receivers who have a low frequency offset.

Use `-t [test_number]` to run a test. To understand what the tests do, please see source code.

Examples: 
---

    welle-cli -c 10B -p GRRIF
    welle-cli -f ./ofdm.iq -p GRRIF
    welle-cli -f ./ofdm.iq -t 1

Limitations
===
* Windows 8 and older are not offically supported

Development
===
You can join the welle.io development. Please visit the [wiki](https://github.com/AlbrechtL/welle.io/wiki) to find more information.
