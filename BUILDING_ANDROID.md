# Building welle.io for Android (Qt 6.5.3)

This document describes the known-good setup to build the Android APK with Qt 6.5.3.

## Requirements

- Qt **6.5.3**
- Android SDK/NDK (tested with NDK r27)
- JDK 17

### Required Qt modules

Install these Qt 6.5.3 modules for **both** `desktop` and `android_arm64_v8a`:

- `qtcharts`
- `qtmultimedia`
- `qtconnectivity`
- `qtpositioning`
- `qtserialport`

> Note: the Android build currently targets `arm64-v8a`.

## Environment

Set the following environment variables (examples use the local Qt install at `~/qt-cli/6.5.3`):

```bash
export ANDROID_SDK_ROOT="$HOME/Android/Sdk"
export ANDROID_HOME="$HOME/Android/Sdk"

export QT_ANDROID_PREFIX_ARM64="$HOME/qt-cli/6.5.3/android_arm64_v8a"
export QT_HOST_PATH="$HOME/qt-cli/6.5.3/gcc_64"
export QT_CMAKE_BIN="$QT_HOST_PATH/bin/qt-cmake"
```

## Install Qt modules (aqtinstall)

If you used `aqtinstall` to install Qt:

```bash
python3 -m venv ~/.venvs/aqt
~/.venvs/aqt/bin/pip install -U pip aqtinstall

~/.venvs/aqt/bin/aqt install-qt linux desktop 6.5.3 gcc_64 \
  -m qtcharts qtmultimedia qtconnectivity qtpositioning qtserialport \
  --outputdir ~/qt-cli

~/.venvs/aqt/bin/aqt install-qt linux android 6.5.3 android_arm64_v8a \
  -m qtcharts qtmultimedia qtconnectivity qtpositioning qtserialport \
  --outputdir ~/qt-cli
```

## Build steps

From the repo root:

```bash
QT_ANDROID_PREFIX_ARM64="$HOME/qt-cli/6.5.3/android_arm64_v8a" \
QT_HOST_PATH="$HOME/qt-cli/6.5.3/gcc_64" \
QT_CMAKE_BIN="$HOME/qt-cli/6.5.3/gcc_64/bin/qt-cmake" \
./tools/android/build.sh
```

The APK is written to:

```
./dist/welle-io-arm64-v8a.apk
```

If a debug keystore exists at `~/.android/debug.keystore`, the script will sign the APK for local install convenience.

## Install to device

```bash
adb install -r dist/welle-io-arm64-v8a.apk
```
