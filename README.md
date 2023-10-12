[welle.io](https://www.welle.io)
=====================
- Linux, macOS (Travis): [![Travis Build Status](https://app.travis-ci.com/AlbrechtL/welle.io.svg?branch=next)](https://app.travis-ci.com/AlbrechtL/welle.io)
- Windows (AppVeyor): [![AppVeyor Build status](https://ci.appveyor.com/api/projects/status/yipsu95pb4ecdofe?svg=true)](https://ci.appveyor.com/project/AlbrechtL/welle-io)

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
    * [Debian / Ubuntu Linux](#debian--ubuntu-linux)
    * [Windows 10 / 11](#windows-10--11)
    * [macOS](#macos)
    * [CMake instead of Qt Creator (Windows, Linux, macOS)](#cmake-instead-of-qt-creator-windows-linux-macos)
    * [Android](#android)
  * [welle-cli](#welle-cli)
    * [Usage](#usage-of-welle-cli)
    * [Backend options](#backend-options)
    * [Examples](#examples)
  * [Limitations](#limitations)
  * [Development](#development)

Download
========
### Stable binaries
* [**Windows**, **Linux**, **macOS** and **Android**](http://github.com/AlbrechtL/welle.io/releases) 
* **Debian** or **Ubuntu** 19.04+
  * `apt install welle.io`, see the /usr/share/doc/welle.io/README.Debian for maintainer notes
* **macOS** (requires [MacPorts](https://www.macports.org/)) 
   * `sudo port install welle.io`
   * `sudo port install welle.io +cli` (if you wish to install also welle-cli)
* **FreeBSD**
  * Building from sources (requires ports tree to be checked out at `/usr/ports/`)
    ```
    cd /usr/ports/audio/welle.io/
    make install clean
    ```
  * Installing the binary package
    ```
    pkg install welle.io`
    ```
* [**Android at Google Play**](https://play.google.com/store/apps/details?id=io.welle.welle) (outdated)

If you discovered an issue please open a new [issue](https://github.com/AlbrechtL/welle.io/issues).

### Unstable developer version
welle.io is under development. You can also try the latest developer builds. But PLEASE BE WARNED the builds are automatically created and untested.

* [welle.io nightly builds](https://welle-io-nightlies.albrechtloh.de/) (Windows, Linux, macOS, Android)

* macOS: welle.io devel builds on *macOS MacPorts* are updated periodically manually and can be installed through [port welle.io-devel](https://ports.macports.org/port/welle.io-devel/summary). The port has no maintainer so please feel free to update it yourself in case you need to use a more recent devel version
  * `sudo port install welle.io-devel`

Usage
=====
The command-line parameters are:

Parameter | Description
------ | ----------
-h, --help | Show help 
-v, --version | Show version 
--dump-file | Records DAB frames (*.mp2) or DAB+ superframes with RS coding (*.dab). This file can be used to analyse X-PAD data with XPADxpert (https://www.basicmaster.de/xpadxpert).
--log-file | Log file name. Redirects all log output texts to a file.

Keyboard shortcuts & hotkeys

Keystroke | Action
------ | ----------
F1-F12, 1-9, 0, Ctrl+1-9, Ctrl+0 | Play the station no. 'x' in the stations list: <br />`1` for station no. `1`, <br />`0` for station no. `10`, <br />`Ctrl+1` for station no. `11`...
S, Media Play, Media Stop, Media Pause, Media Play/Pause | Start playback/Stop
N, Media next | play next station in list
P, Media Previous | play previous station
M, Volume Mute | mute/unmute
Ctrl+Up, Volume Up | Volume Up
Ctrl+Down, Volume Down | Volume Down

Supported Hardware
====================
The following SDR devices are supported
* Airspy R2 and Airspy Mini (http://airspy.com/)
* rtl-sdr (http://osmocom.org/projects/sdr/wiki/rtl-sdr)
* rtl_tcp (http://osmocom.org/projects/sdr/wiki/rtl-sdr#rtl_tcp)
* I/Q RAW file (https://www.welle.io/devices/rawfile)
* All SDR-devices that are supported by SoapySDR, gr-osmosdr and uhd. These are too many devices to list them all. To see if your SDR is supported, have a look at the lists at [SoapySDR](https://github.com/pothosware/SoapySDR/wiki) and [SoapyOsmo](https://github.com/pothosware/SoapyOsmo/wiki).
    * Devices supported by gr-osmosdr are supported via [SoapyOsmo](https://github.com/pothosware/SoapyOsmo/wiki)
    * Devices supported by uhd are supported via [SoapyUHD](https://github.com/pothosware/SoapyUHD/wiki)
    * One limitation is of course that the SDR devices must be tunable to the DAB+ frequencies.

### SoapySDR Notes


#### LimeSDR

Connect the antenna to the RX1_W port and configured SoapySDR antenna option to `LNAW`. `SoapySDRUtil --probe=driver=lime` may show other possible options.

#### USRP

Configured SoapySDR driver arguments option to `driver=uhd`. Configure also antenna and clock source option. To list possible values for antenna and clock source use the command `SoapySDRUtil --probe=driver=uhd`.

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

Debian / Ubuntu Linux
---
This section shows how to compile welle.io on Debian or Ubuntu (tested with Ubuntu 20.04).

1. Install the base requirements

```
sudo apt install git build-essential
```

2. Install the following non-Qt packages

```
sudo apt install libfaad-dev libmpg123-dev libfftw3-dev librtlsdr-dev libusb-1.0-0-dev mesa-common-dev libglu1-mesa-dev libpulse-dev libsoapysdr-dev libairspy-dev libmp3lame-dev libflac++-dev
```

3. Install the following Qt packages

```
sudo apt install libqt5charts5-dev qtbase5-dev qttools5-dev-tools qtquickcontrols2-5-dev libqt5quick5 qtdeclarative5-dev qtmultimedia5-dev libqt5quick5 libqt5multimedia5-plugins qml-module-qt-labs-settings qml-module-qtquick-window2 qml-module-qtquick2 qml-module-qtquick-layouts qml-module-qtquick-dialogs qml-module-qtquick-controls2 qml-module-qtquick-controls qml-module-qtquick-templates2 qml-module-qtquick-privatewidgets qml-module-qtquick-localstorage qml-module-qtcharts qml-module-qtgraphicaleffects qml-module-qt-labs-folderlistmodel qtcreator
```

4. Clone welle.io

```
git clone https://github.com/AlbrechtL/welle.io.git
```

5. Start Qt Creator and open the project file `welle.io.pro` inside the folder "welle.io".
6. Build welle.io
7. Run welle.io and enjoy it

Windows 10 / 11
---
A compiled version can be found at the [release page](https://github.com/AlbrechtL/welle.io/releases)

This sections shows how to compile welle.io on Windows 10. Windows 7 should also be possible but is not tested. 

1. Install Qt 5.10 including the Qt Charts and mingw modules by using the "Qt Online Installer for Windows" https://www.qt.io/download-open-source/
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
brew tap AlbrechtL/welle_io https://github.com/AlbrechtL/welle.io
```

2. Install welle.io (and dependencies):

```
brew install AlbrechtL/welle_io/welle.io
```

### MacPorts

You can either use the welle.io port available in macports, or compile with QT Creator.

#### use welle.io port

This is the easiest way and will manage the dependencies for you.
Variants enabled by default are : "airspy" "rtlsdr" "soapysdr". Each enables compilation with that specific input device library.
```
sudo port install welle.io
```
Additional variants are : "cli" (to install also welle-cli) "profiling" & "kiss_fft".

With MacPorts, welle.io is installed as a bundle app in `/Applications/MacPorts`.

You can also use welle.io-devel port if you prefer:
```
sudo port install welle.io-devel
```

#### compile with QT Creator

You need to install the dependencies with MacPorts first, assuming you have [MacPorts](https://www.macports.org/) installed:

```
sudo port install fftw-3-single faad2 rtl-sdr libusb mpg123 lame
```

1. Install Qt 5.10 with Qt Creator directly from Qt website, not through MacPorts.
2. Clone welle.io

```
git clone https://github.com/AlbrechtL/welle.io.git
```

3. Open welle.io.pro with Qt Creator.
4. Make sure in Qt Creator, "Projects, Build&Run, Run" that the checkbox "Add build library path to DYLD..." is off.
5. Build and run.

FreeBSD
---
This section describes how to build welle.io from sources on FreeBSD 12.2 and 13.0.

1. You will need the following dependencies, either built from the
   ports or installed as a binary package. You may also build them
   yourself.

```
pkg install alsa-lib faad lame mpg123 pkgconf cmake qt5-charts \
    qt5-core qt5-declarative qt5-gui qt5-multimedia qt5-network \
    qt5-quickcontrols2 qt5-widgets qt5-buildtools qt5-qmake \
    rtl-sdr fftw3-float fftw3
```
   For SoapySDR support, you will also need `soapysdr`. For AirSpy support, you will need `airspy`.

2. Now follow the build instructions for CMake as indicated below.


CMake instead of Qt Creator (Windows, Linux, macOS, FreeBSD)
---

As an alternative to Qt Creator, CMake can be used for building welle.io after installing dependencies and cloning the repository. On Linux, you can also use CMake to build [**welle-cli**](#welle-cli), the command-line backend testing tool that does not require Qt.

1. Create a build directory inside the repository and change into it

```
mkdir build
cd build
```

2. Run CMake. To enable support for RTL-SDR add the flag `-DRTLSDR=1` (requires librtlsdr) and for SoapySDR add `-DSOAPYSDR=1` (requires SoapySDR compiled with support for each desired hardware, e.g. UHD for Ettus USRP, LimeSDR, Airspy or HackRF). By default, CMake will build both welle-io and welle-cli. Use `-DBUILD_WELLE_IO=OFF` or `-DBUILD_WELLE_CLI=OFF` to compile only the one you need.

```
cmake ..
```

  or to enable support for both RTL-SDR and Soapy-SDR:

```
cmake .. -DRTLSDR=1 -DSOAPYSDR=1
```

  If you wish to use KISS FFT instead of FFTW (e.g. to compare performance), use `-DKISS_FFT=ON`.

3. Run make (or use the created project file depending on the selected generator)

```
make
```

4. Install it (as super-user)

```
make install
```

5. Run welle.io and enjoy it

Android
---
A compiled version of welle.io (APK file) can be found at at the [Google Play store](https://play.google.com/store/apps/details?id=io.welle.welle) or at the [release page](https://github.com/AlbrechtL/welle.io/releases).

welle.io uses the "RTL2832U driver" from Martin Marinov, to be found at the [Google play store](https://play.google.com/store/apps/details?id=marto.rtl_tcp_andro) or at [F-droid](https://f-droid.org/packages/marto.rtl_tcp_andro/). Also see ([sources](https://github.com/martinmarinov/rtl_tcp_andro-) or [APK file](https://github.com/martinmarinov/rtl_tcp_andro-/blob/master/app/app-release.apk)). Please note that a recent version of this driver is needed (v3.06 or above), otherwise welle.io will not find your stick.

This sections shows how to compile welle.io for Android.

1. Install Qt 5.12 for Android including the Qt Charts and Qt Remote Objects modules by using the "Qt Online Installer for Windows" https://www.qt.io/download-open-source/
2. Follow the side https://doc.qt.io/qt-5/androidgs.html to install the Android build environment
3. Clone welle.io https://github.com/AlbrechtL/welle.io.git

```
git clone https://github.com/AlbrechtL/welle.io.git
```
  
4. Start Qt Creator and open the project file `welle.io.pro` inside the folder "welle.io".
5. Build welle.io
6. Run welle.io and enjoy it

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

Streaming output options
---

By default, `welle-cli` will output in mp3 if in webserver mode.
With the `-O` option, you can choose between mp3 and flac (lossless) if FLAC support is enabled at build time.

Backend options
---

`-u` disable coarse corrector, for receivers who have a low frequency offset.

Use `-t [test_number]` to run a test. To understand what the tests do, please see source code.

Driver options
---

By default, `welle-cli` tries all enabled drivers in turn and uses the first device it can successfully open.

Use `-F [driver][,driver_args]` to select a specific driver and optionally pass arguments to the driver.
This allows to select the `rtl_tcp` driver (which is not autodetected) and pass the hostname or IP address and port of the rtl_tcp server to it:

    welle-cli -C 10B -p GRRIF -F rtl_tcp,192.168.12.34:1234
    welle-cli -C 10B -P GRRIF -F rtl_tcp,my.rtl-tcp.local:9876

Right now, `rtl_tcp` is the only driver that accepts options from the command line.

Examples: 
---

    welle-cli -c 10B -p GRRIF
    welle-cli -f ./ofdm.iq -p GRRIF
    welle-cli -f ./ofdm.iq -t 1

Limitations
===
* Windows 8 and older are not officially supported

Development
===
You can join the welle.io development. Please visit the [wiki](https://github.com/AlbrechtL/welle.io/wiki) to find more information.

Profiling
---
If you build with cmake and add `-DPROFILING=ON`, welle-io will generate a few `.csv` files and a graphviz `.dot` file that can be used
to analyse and understand which parts of the backend use CPU resources. Use `dot -Tpdf profiling.dot > profiling.pdf` to generate a graph
visualisation. Search source code for the `PROFILE()` macro to see where the profiling marks are placed.
