# Building

## General information

The following libraries and their development files are needed:

* Qt 6.8 or newer (with Qt Charts and Qt Multimedia)
* libfftw3
* libfaad
* librtlsdr
* libusb
* libmpg123
* libairspy
* libmp3lame
* libsoapysdr

welle-cli (`-DBUILD_WELLE_IO=OFF`) doesn't need Qt.

## Debian / Ubuntu Linux

This section shows how to compile welle.io on Debian or Ubuntu (tested with
Ubuntu 24.04).

1. Install the base requirements

    ```sh
    sudo apt install git build-essential
    ```

2. Install the following non-Qt packages

    ```sh
    sudo apt install libfaad-dev libmpg123-dev libfftw3-dev librtlsdr-dev libusb-1.0-0-dev mesa-common-dev libglu1-mesa-dev libpulse-dev libsoapysdr-dev libairspy-dev libmp3lame-dev libflac++-dev xxd
    ```

3. Install Qt with the
   [Qt online installer](https://www.qt.io/download-qt-installer-oss). It is
   recommended to use the newest Qt version. Besides Qt you need the
   additional Qt libraries:
    * Qt Charts
    * Qt Multimedia

4. Clone welle.io

    ```sh
    git clone https://github.com/AlbrechtL/welle.io.git
    ```

5. Start Qt Creator and open the project file `welle.io.pro` in the folder
   `welle.io`, or build with [CMake](#cmake).
6. Build welle.io
7. Run welle.io and enjoy it

## Windows 11

This section shows how to compile welle.io on Windows 11. Windows 10 should
also work but is not tested.

1. Install Qt with the
   [Qt online installer](https://www.qt.io/download-qt-installer-oss). It is
   recommended to use the newest Qt version. Besides Qt you need the
   additional Qt libraries:
    * Qt Charts
    * Qt Multimedia
2. Clone welle.io <https://github.com/AlbrechtL/welle.io.git>, e.g. with
   [TortoiseGit](https://tortoisegit.org).
3. Clone the welle.io Windows libraries
   <https://github.com/AlbrechtL/welle.io-win-libs.git>.
4. Start Qt Creator and open the project file `welle.io.pro` in the folder
   `welle.io`.
5. Build welle.io
6. Run welle.io and enjoy it

## macOS

The CMake build is tested on the macOS 15 and 26 GitHub-hosted runners for
both Apple Silicon (ARM64) and Intel (AMD64/x86_64). Install the build
dependencies with [Homebrew](https://brew.sh/):

```sh
brew install cmake ninja qt fftw faad2 mpg123 lame librtlsdr dylibbundler
```

Configure and build the application bundle and the command line tool:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qt)" \
  -DRTLSDR=ON
cmake --build build --parallel
```

The resulting application is `build/welle-io.app`. To make a self-contained
bundle for another Mac, deploy its Qt and Homebrew libraries and apply an
ad-hoc signature:

```sh
scripts/macos-deploy.sh "$(brew --prefix qt)"
```

GitHub Actions performs this build on every supported macOS runner. Each
deployed application bundle is archived as
`YYYYMMDD_GITHASH_macOSVERSION_welle-io_ARCH.zip` and kept as a workflow
artifact. Pushes to `master` or branches starting with `next` also upload the
package to the nightly server when its credentials are configured. As these
CI packages use an ad-hoc signature, macOS may require the user to approve
them in *Privacy & Security* before opening. A stable release still requires
Developer ID signing and Apple notarization and remains a manual release
step.

## Android and FreeBSD

These operating systems are not maintained at the moment. You can find the
original build instructions in the old
[README.md](https://github.com/AlbrechtL/welle.io/blob/fdcd3c588a6e592b9640aad71648dcf6228fa98f/README.md).

## CMake

As an alternative to Qt Creator, CMake can be used to build welle.io after
installing the dependencies and cloning the repository. On Linux, you can
also use CMake to build [welle-cli](../user-guide/welle-cli.md), the command
line tool that does not require Qt.

1. Create a build directory inside the repository and change into it

    ```sh
    mkdir build
    cd build
    ```

2. Run CMake. To enable support for rtl-sdr add the flag `-DRTLSDR=1`
   (requires librtlsdr) and for SoapySDR add `-DSOAPYSDR=1` (requires
   SoapySDR compiled with support for each desired hardware, e.g. UHD for
   Ettus USRP, LimeSDR, Airspy or HackRF). By default, CMake builds both
   welle-io and welle-cli. Use `-DBUILD_WELLE_IO=OFF` or
   `-DBUILD_WELLE_CLI=OFF` to compile only the one you need.

    ```sh
    cmake ..
    ```

    or to enable support for both rtl-sdr and SoapySDR:

    ```sh
    cmake .. -DRTLSDR=1 -DSOAPYSDR=1
    ```

    If you wish to use KISS FFT instead of FFTW (e.g. to compare
    performance), use `-DKISS_FFT=ON`.

    !!! tip "Qt installed with the Qt online installer"
        The online installer does not put Qt into a system path, so CMake
        fails with `Could not find a package configuration file provided by
        "Qt6"`. Tell CMake where Qt is installed by setting
        `CMAKE_PREFIX_PATH` to the kit directory
        (`<Qt install dir>/<version>/<kit>`). The default install directory
        is `~/Qt` (Linux and macOS) or `C:\Qt` (Windows).

        Make sure the following components are selected in the installer for
        your Qt version: *Desktop (gcc_64 / macOS / MSVC or MinGW)*,
        *Qt Multimedia* and *Qt Charts* (under *Additional Libraries*). You
        can add missing components later with the *Qt Maintenance Tool*.

        === "Linux"

            ```sh
            cmake .. -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/gcc_64
            ```

        === "macOS"

            ```sh
            cmake .. -DCMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/macos
            ```

        === "Windows (MinGW)"

            Run in the *Qt 6.12.0 (MinGW)* command prompt:

            ```bat
            cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:\Qt\6.12.0\mingw_64
            ```

        Alternatively set the environment variable once:
        `export CMAKE_PREFIX_PATH=$HOME/Qt/6.12.0/gcc_64`. If you have
        changed the prefix path after a failed run, delete the `build`
        directory (or `CMakeCache.txt`) first, because CMake caches the
        failed lookup.

3. Run make (or use the created project file, depending on the selected
   generator)

    ```sh
    make
    ```

4. Install it (as super-user)

    ```sh
    make install
    ```

5. Run welle.io and enjoy it

## Tests

`-DBUILD_TESTS=ON` builds the automated tests; run them with
`ctest --test-dir build`. See
[src/tests/README.md](https://github.com/AlbrechtL/welle.io/blob/master/src/tests/README.md).

## Profiling

If you build with CMake and add `-DPROFILING=ON`, welle-io generates a few
`.csv` files and a graphviz `.dot` file that can be used to analyse and
understand which parts of the backend use CPU resources. Use
`dot -Tpdf profiling.dot > profiling.pdf` to generate a graph visualisation.
Search the source code for the `PROFILE()` macro to see where the profiling
marks are placed.
