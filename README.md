[welle.io](https://www.welle.io)
=====================
welle.io is an open source DAB and DAB+ software defined radio (SDR) for Windows, Linux, macOS and the Raspberry Pi. It supports rtl-sdr (RTL2832U), Airspy, rtl_tcp, SoapySDR and I/Q raw files.

The documentation is on the project website **https://www.welle.io**. Its source is in [docs/](docs/).

**Build status**
- Linux (Flatpak x86_64 and arm64): [![Linux build](https://github.com/AlbrechtL/welle.io/actions/workflows/linux.yml/badge.svg)](https://github.com/AlbrechtL/welle.io/actions/workflows/linux.yml)
- macOS 15 (also tested on 26 and 27): [![macOS build](https://github.com/AlbrechtL/welle.io/actions/workflows/macos.yml/badge.svg)](https://github.com/AlbrechtL/welle.io/actions/workflows/macos.yml)
- Windows (Installer x86_64): [![Windows build](https://github.com/AlbrechtL/welle.io/actions/workflows/windows.yml/badge.svg)](https://github.com/AlbrechtL/welle.io/actions/workflows/windows.yml)
- Android (APK): build workflow is currently disabled because a new, skilled maintainer for Android package is required to fix bug https://github.com/AlbrechtL/welle.io/issues/814# in the workflow.


## Download

* **Windows**: [GitHub releases](https://github.com/AlbrechtL/welle.io/releases)
* **Linux**: [Flatpak on Flathub](https://flathub.org/apps/io.welle.welle-gui) (recommended) or the [packages of your distribution](https://repology.org/project/welle.io/versions)
* **macOS**, **FreeBSD** and more: see [Installation](docs/user-guide/installation.md)
* **Developer version**: [welle.io nightly builds](https://welle-io-nightlies.albrechtloh.de/) (Windows, Linux, macOS). PLEASE BE WARNED: they are created automatically and untested.

If you discovered an issue please open a new [issue](https://github.com/AlbrechtL/welle.io/issues).

## Documentation

* [Usage](docs/user-guide/usage.md): command line options and keyboard shortcuts
* [Supported hardware](docs/devices/index.md)
* [welle-cli](docs/user-guide/welle-cli.md): the command line tool and its web interface, also as a systemd service
* [FAQ](docs/faq.md)

## Building

Quick start on Linux with CMake (Qt 6.8 or newer, see [Building](docs/development/building.md) for the dependencies and for Windows and macOS):

```
cmake -B build -DRTLSDR=ON -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/gcc_64
cmake --build build -j$(nproc)
```

## Development

You can join the welle.io development. Please visit the [development documentation](docs/development/index.md) to find more information.

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
