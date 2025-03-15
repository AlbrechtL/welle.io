[welle.io](https://www.welle.io)
=====================
This repository contains the implementation of an SDR DAB/DAB+ receiver.  
Please see the project website https://www.welle.io for a user oriented documentation.

**Build status**
- Linux (Flatpak x86_64 and arm64): [![Linux build](https://github.com/AlbrechtL/welle.io/actions/workflows/linux.yml/badge.svg)](https://github.com/AlbrechtL/welle.io/actions/workflows/linux.yml)
- Windows (Installer x86_64): [![Windows build](https://github.com/AlbrechtL/welle.io/actions/workflows/windows.yml/badge.svg)](https://github.com/AlbrechtL/welle.io/actions/workflows/windows.yml)
- Android (APK): build workflow is currently disabled because a new, skilled maintainer for Android package is required to fix bug https://github.com/AlbrechtL/welle.io/issues/814# in the workflow.


## Table of contents

  * [Download](#download)
  * [Usage](#usage)
  * [Supported Hardware](#supported-hardware)
  * [Building](#building)
    * [General Information](#general-information)
    * [Debian / Ubuntu Linux](#debian--ubuntu-linux)
    * [Windows 11](#windows-11)
    * [macOS, Android and FreeBSD](#macos-android-and-freebsd)
    * [CMake](#cmake)
  * [welle-cli](#welle-cli)
    * [Usage](#usage-of-welle-cli)
    * [Backend options](#backend-options)
    * [Examples](#examples)
  * [Limitations](#limitations)
  * [Development](#development)
  * [Acknowledgement](#acknowledgement)
  * [Sponsors](#sponsors)

## Download

#### Stable binaries

* [**Windows**](http://github.com/AlbrechtL/welle.io/releases)
* **Linux** (flatpak version is recommend because Linux distribution packages welle.io can be outdated.
  - [Flatpak](https://flathub.org/apps/io.welle.welle-gui) 
  - [Full list of supported Linux distributions](https://repology.org/project/welle.io/versions)
  - Debian or Ubuntu 19.04+:
    ```
    $ apt install welle.io`
    ```
  - Fedora 35+ (uses the [RPM Fusion](https://rpmfusion.org) package sources)
    ```
    # Enable the RPM Fusion sources
    $ sudo dnf install -y https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm https://mirrors.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm` 

    # Install welle.io
    $ sudo dnf install --refresh welle-io
    ```
* **macOS**
  Unfortunately the macOS welle.io is unmaintained, currently. You still can use the 2.4 version (only Intel processor support).
  - [Installer](https://github.com/AlbrechtL/welle.io/releases/tag/v2.4)
  - MacPorts
    ```
    $ sudo port install welle.io
    ```
* **[Android](https://github.com/AlbrechtL/welle.io/releases/tag/v2.4)** (Unfortunately unmaintained, currently. You still can use the 2.4 version)
* **FreeBSD**
  ```
  $ pkg install welle.io
  ```
  
If you discovered an issue please open a new [issue](https://github.com/AlbrechtL/welle.io/issues).

#### Developer version

welle.io is under development. You can also try the latest developer builds. But PLEASE BE WARNED the builds are automatically created and untested.
* [welle.io nightly builds](https://welle-io-nightlies.albrechtloh.de/) (Windows, Linux)
* Build artifacts in [Actions](https://github.com/AlbrechtL/welle.io/actions) runs


## Usage

Since welle.io is a GUI program simply start it.

#### Command-line parameters

Parameter | Description
------ | ----------
-h, --help | Show help 
-v, --version | Show version 
--dump-file | Records DAB frames (*.mp2) or DAB+ superframes with RS coding (*.dab). This file can be used to analyse X-PAD data with XPADxpert (https://www.basicmaster.de/xpadxpert).
--log-file | Log file name. Redirects all log output texts to a file.

#### Keyboard shortcuts & hotkeys

Keystroke | Action
------ | ----------
F1-F12, 1-9, 0, Ctrl+1-9, Ctrl+0 | Play the station no. 'x' in the stations list: <br />`1` for station no. `1`, <br />`0` for station no. `10`, <br />`Ctrl+1` for station no. `11`...
S, Media Play, Media Stop, Media Pause, Media Play/Pause | Start playback/Stop
N, Media next | play next station in list
P, Media Previous | play previous station
M, Volume Mute | mute/unmute
Ctrl+Up, Volume Up | Volume Up
Ctrl+Down, Volume Down | Volume Down

## Supported Hardware

The following SDR devices are supported
* RTL2832U
  - [rtl-sdr](http://osmocom.org/projects/sdr/wiki/rtl-sdr)
  - [rtl_tcp](http://osmocom.org/projects/sdr/wiki/rtl-sdr#rtl_tcp)
* [Airspy R2 and Airspy Mini ](http://airspy.com/)
  - Limitation: Airspy HF+ is not supported due to limited bandwidth
* [I/Q RAW file](https://www.welle.io/devices/rawfile)
* [SoapySDR](https://github.com/pothosware/SoapySDR/): All SDR-devices that are supported by SoapySDR, gr-osmosdr and uhd. These are too many devices to list them all. To see if your SDR is supported, have a look at the lists at [SoapySDR](https://github.com/pothosware/SoapySDR/wiki) and [SoapyOsmo](https://github.com/pothosware/SoapyOsmo/wiki).
    * Devices supported by gr-osmosdr are supported via [SoapyOsmo](https://github.com/pothosware/SoapyOsmo/wiki)
    * Devices supported by uhd are supported via [SoapyUHD](https://github.com/pothosware/SoapyUHD/wiki)
    * Notes
      - LimeSDR: Connect the antenna to the RX1_W port and configured SoapySDR antenna option to `LNAW`. `SoapySDRUtil --probe=driver=lime` may show other possible options.
      - USRP: Configured SoapySDR driver arguments option to `driver=uhd`. Configure also antenna and clock source option. To list possible values for antenna and clock source use the command `SoapySDRUtil --probe=driver=uhd`.
      - One limitation is of course that the SDR devices must be tunable to the DAB+ frequencies.

## Building

#### General Information

The following libraries and their development files are needed.

* Qt 6.8 or above
* libfftw3
* libfaad
* librtlsdr
* libusb
* libmpg123
* libairspy
* libmp3lame
* libsoapysdr


#### Debian / Ubuntu Linux

This section shows how to compile welle.io on Debian or Ubuntu (tested with Ubuntu 24.04).

1. Install the base requirements

```
sudo apt install git build-essential
```

2. Install the following non-Qt packages

```
sudo apt install libfaad-dev libmpg123-dev libfftw3-dev librtlsdr-dev libusb-1.0-0-dev mesa-common-dev libglu1-mesa-dev libpulse-dev libsoapysdr-dev libairspy-dev libmp3lame-dev libflac++-dev xxd
```

3. Install the Qt via the [Qt online installer](https://www.qt.io/download-qt-installer-oss). It is recommend to use the newest Qt version. Besids Qt you need the additional Qt libraries:
* Qt 5 Compatibility Module
* Qt Charts
* Qt Multimedia

4. Clone welle.io

```
git clone https://github.com/AlbrechtL/welle.io.git
```

5. Start Qt Creator and open the project file `welle.io.pro` inside the folder "welle.io".
6. Build welle.io
7. Run welle.io and enjoy it

#### Windows 11


This section shows how to compile welle.io on Windows 11. Windows 10 and 7  should also be possible but is not tested. 

1. Install the Qt via the [Qt online installer](https://www.qt.io/download-qt-installer-oss). It is recommend to use the newest Qt version. Besids Qt you need the additional Qt libraries:
* Qt 5 Compatibility Module
* Qt Charts
* Qt Multimedia
2. Clone welle.io https://github.com/AlbrechtL/welle.io.git e.g. by using [TortoiseGit](https://tortoisegit.org).
3. Clone the welle.io Windows libraries https://github.com/AlbrechtL/welle.io-win-libs.git.
4. Start Qt Creator and open the project file `welle.io.pro` inside the folder "welle.io".
5. Build welle.io
6. Run welle.io and enjoy it

#### macOS, Android and FreeBSD

These operating systems are not maintain, currently. You can find the original compiling insturections in the old [README.md](https://github.com/AlbrechtL/welle.io/blob/fdcd3c588a6e592b9640aad71648dcf6228fa98f/README.md).


#### CMake

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


## welle-cli

If you compile welle-io with [`cmake`](#cmake-instead-of-qt-creator-windows-linux-macos) you will also get an executable called **welle-cli** which stands for welle-io **c**ommand **l**ine **i**nterface. 

#### Usage of welle-cli 


Receive using RTLSDR, and play with ALSA:

    `welle-cli -c channel -p programme`

Read an IQ file and play with ALSA: (IQ file format is u8, unless the file ends with FORMAT.iq)

    `welle-cli -f file -p programme`

Use -D to dump FIC and all programmes to files:
 
    `welle-cli -c channel -D` 

Use -w to enable webserver, decode a programme on demand:
    
    `welle-cli -c channel -w port`

Use -Dw to enable webserver, decode all programmes:
    
    `welle-cli -c channel -Dw port`

Use `-C 1 -w` to enable webserver, decode programmes one by one in a carousel.
Use `-C N -w` to enable webserver, decode programmes N by N in a carousel.
This is useful if your machine cannot decode all programmes simultaneously, but you still want to get an overview of the ensemble.
By default, welle-cli will switch every 10 seconds.
With the `-P` option, welle-cli will switch once DLS and a slide were decoded, staying at most for 80 seconds on a given programme.

    welle-cli -c channel -C 1 -w port
    welle-cli -c channel -PC 1 -w port
    
Example: `welle-cli -c 12A -C 1 -w 7979` enables the webserver on channel 12A, please then go to http://localhost:7979/ where you can observe all necessary details for every service ID in the ensemble, see the slideshows, stream the audio (by clicking on the Play-Button), check spectrum, constellation, TII information and CIR peak diagramme.

#### Streaming output options

By default, `welle-cli` will output in mp3 if in webserver mode.
With the `-O` option, you can choose between mp3 and flac (lossless) if FLAC support is enabled at build time.

#### Backend options

`-u` disable coarse corrector, for receivers who have a low frequency offset.

Use `-t [test_number]` to run a test. To understand what the tests do, please see source code.

#### Driver options

By default, `welle-cli` tries all enabled drivers in turn and uses the first device it can successfully open.

Use `-F [driver][,driver_args]` to select a specific driver and optionally pass arguments to the driver.
This allows to select the `rtl_tcp` driver (which is not auto-detected) and pass the hostname or IP address and port of the rtl_tcp server to it:

    welle-cli -C 10B -p GRRIF -F rtl_tcp,192.168.12.34:1234
    welle-cli -C 10B -P GRRIF -F rtl_tcp,my.rtl-tcp.local:9876

Right now, `rtl_tcp` is the only driver that accepts options from the command line.

**Examples**: 


    welle-cli -c 10B -p GRRIF
    welle-cli -f ./ofdm.iq -p GRRIF
    welle-cli -f ./ofdm.iq -t 1

## Limitations

* Windows 10 and older are not officially supported.

## Development

You can join the welle.io development. Please visit the [wiki](https://github.com/AlbrechtL/welle.io/wiki) to find more information.

## Profiling

If you build with cmake and add `-DPROFILING=ON`, welle-io will generate a few `.csv` files and a graphviz `.dot` file that can be used
to analyse and understand which parts of the backend use CPU resources. Use `dot -Tpdf profiling.dot > profiling.pdf` to generate a graph
visualisation. Search source code for the `PROFILE()` macro to see where the profiling marks are placed.

## Acknowledgement


I would like to thanks to following open source projects. Without these great works this program would not be possible
* [qt-dab](https://github.com/JvanKatwijk/qt-dab) (In 2017 we forked welle.io from  qt-dab)
* [dablin](https://github.com/Opendigitalradio/dablin)
* [Opendigitalradio](https://www.opendigitalradio.org/)
* FFTW
* Qt

## Sponsors


<table>
 <tbody>
  <tr>
   <td align="center"><img alt="[SignPath]" src="https://avatars.githubusercontent.com/u/34448643" height="30"/></td>
   <td>Free code signing on Windows provided by <a href="https://signpath.io/?utm_source=foundation&utm_medium=github&utm_campaign=welle">SignPath.io</a>, certificate by <a href="https://signpath.org/?utm_source=foundation&utm_medium=github&utm_campaign=welle">SignPath Foundation</a></td>
  </tr>
 </tbody>
</table>
