# Installation

## Stable versions

### Windows

Download the installer from the
[GitHub releases](https://github.com/AlbrechtL/welle.io/releases). To use an
rtl-sdr stick, install its driver with Zadig, see
[rtl-sdr](../devices/rtl_sdr.md#windows-set-up).

Windows 10 and older are not officially supported.

### Linux

The Flatpak version is recommended, because the packages of the Linux
distributions can be outdated.

* [Flatpak on Flathub](https://flathub.org/apps/io.welle.welle-gui)
* [Full list of Linux distributions with welle.io](https://repology.org/project/welle.io/versions)
* Debian or Ubuntu 19.04 and newer:

    ```sh
    sudo apt install welle.io
    ```

* Fedora 35 and newer (uses the [RPM Fusion](https://rpmfusion.org) package
  sources):

    ```sh
    # Enable the RPM Fusion sources
    sudo dnf install -y https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm https://mirrors.rpmfusion.org/nonfree/fedora/rpmfusion-nonfree-release-$(rpm -E %fedora).noarch.rpm

    # Install welle.io
    sudo dnf install --refresh welle-io
    ```

### macOS

The current source tree builds on both Apple Silicon and Intel Macs, see the
[macOS build instructions](../development/building.md#macos) and the
[nightly builds](#nightly-builds).

* [Legacy 2.4 Intel installer](https://github.com/AlbrechtL/welle.io/releases/tag/v2.4)
* MacPorts:

    ```sh
    sudo port install welle.io
    ```

### Android

The Android version is unmaintained at the moment. You can still use the
[2.4 version](https://github.com/AlbrechtL/welle.io/releases/tag/v2.4).

### FreeBSD

```sh
pkg install welle.io
```

## Nightly builds

welle.io is under development. You can also try the latest developer builds.

!!! warning
    The nightly builds are created automatically and are untested.

* [welle.io nightly builds](https://welle-io-nightlies.albrechtloh.de/)
  (Windows, Linux, macOS)
* Build artifacts of the [GitHub Actions](https://github.com/AlbrechtL/welle.io/actions) runs

The macOS packages of the nightly builds have an ad-hoc signature, so macOS
may ask you to allow them in *Privacy & Security* before they open.

## Problems?

If you discovered an issue please open a new
[issue](https://github.com/AlbrechtL/welle.io/issues).
