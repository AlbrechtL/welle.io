# Android APK build + install log

Date: 2026-02-22
Host: Pop!_OS (per user), device connected via ADB

## 1) SDK/ADB validation
Commands:
- `printenv ANDROID_SDK_ROOT ANDROID_HOME`
- `which adb sdkmanager apksigner aapt`
- `adb devices`

Results:
- `ANDROID_SDK_ROOT=/home/leonamramosfoli/Android/Sdk`
- `ANDROID_HOME=/home/leonamramosfoli/Android/Sdk`
- `adb=/home/leonamramosfoli/Android/Sdk/platform-tools/adb`
- `sdkmanager=/home/leonamramosfoli/Android/Sdk/cmdline-tools/latest/bin/sdkmanager`
- `apksigner=/home/leonamramosfoli/Android/Sdk/build-tools/34.0.0/apksigner`
- `aapt=/home/leonamramosfoli/Android/Sdk/build-tools/34.0.0/aapt`
- `adb devices`:
  - `adb-644875bb-aa7GxT._adb-tls-connect._tcp  device`

## 2) SDK packages present
Command:
- `sdkmanager --list | sed -n '1,120p'`

Installed (local):
- build-tools: 34.0.0, 35.0.0, 36.1.0
- platform-tools 36.0.2
- platforms;android-34
- ndk;26.1.10909125, ndk;26.3.11579264, ndk;27.0.12077973

## 3) Attempt to install missing packages
Command:
- `yes | sdkmanager "platforms;android-33" "ndk;25.1.8937393"`

Result:
- Failed due to inability to fetch remote manifests (`Failed to download any source lists`).
- `platforms;android-33` not found (offline).

## 4) Qt for Android availability
Checks:
- `command -v qt-cmake` → not found in PATH
- `/usr/lib/qt6/bin/qt-cmake` exists (host Qt tools only)
- No Qt Android SDK/ABI trees found under `/opt`, `/usr/lib/qt6`, or `$HOME`.

## 5) Attempt to install Qt for Android (aqtinstall)
Commands:
- `python3 -m venv ~/.venvs/aqt && ~/.venvs/aqt/bin/pip install -U pip aqtinstall`

Result:
- Failed to reach PyPI (`No address associated with hostname`), so `aqtinstall` not installable offline.

## Current blocker
Building an Android APK requires a Qt for Android ABI installation (e.g. `/opt/Qt/6.5.x/android_arm64_v8a`) which is not present locally and cannot be downloaded without network access to Qt or PyPI.

## Build script created
- `tools/android/build.sh`
  - Reproducible, headless build for `arm64-v8a`
  - Produces `dist/welle-io-arm64-v8a.apk`
  - Requires `QT_ANDROID_PREFIX_ARM64` to be set to an existing Qt Android arm64 prefix.

## Next steps (pending unblock)
1) Provide a local Qt for Android installation path (arm64-v8a), or allow network access to install it.
2) Run `tools/android/build.sh` to generate APK.
3) Validate APK with `apksigner` / `aapt` and install via `adb` to reproduce issue #814.
4) Capture `adb install` error and `adb logcat`, then apply minimal fix.

## 6) Build attempt with local Qt Android
Command:
- `QT_ANDROID_PREFIX_ARM64=/home/leonamramosfoli/Qt/6.5.3/android_arm64_v8a \
   QT_CMAKE_BIN=/home/leonamramosfoli/Qt/6.5.3/gcc_64/bin/qt-cmake \
   QT_HOST_PATH=/home/leonamramosfoli/Qt/6.5.3/gcc_64 \
   tools/android/build.sh`

Result:
- CMake failed: Qt6Multimedia not found in host Qt.
- Missing config:
  - `/home/leonamramosfoli/Qt/6.5.3/gcc_64/lib/cmake/Qt6Multimedia/Qt6MultimediaConfig.cmake`

Verification:
- `ls /home/leonamramosfoli/Qt/6.5.3/gcc_64/lib/cmake | grep Multimedia` → no results
- `ls /home/leonamramosfoli/Qt/6.5.3/android_arm64_v8a/lib/cmake | grep Multimedia` → no results

Blocker:
- Qt6 Multimedia module is missing for both host and Android arm64 Qt installs.
- Build cannot proceed without installing Qt Multimedia for 6.5.3 (host + android_arm64_v8a).

## 7) Auto-install missing Qt module via aqtinstall (attempt)
Detected missing module: Qt6Multimedia

Action:
- `tools/android/install-qt-module.sh Multimedia`

Result:
- Failed because `pip` cannot reach PyPI (no network).
- `ERROR: Could not find a version that satisfies the requirement aqtinstall`

Consequence:
- Automated module install via `aqtinstall` cannot proceed offline.

## 8) Build attempt with qt-cli Qt (clean build dir)
Command:
- `rm -rf build-android-arm64`
- `QT_ANDROID_PREFIX_ARM64=/home/leonamramosfoli/qt-cli/6.5.3/android_arm64_v8a \
   QT_CMAKE_BIN=/home/leonamramosfoli/qt-cli/6.5.3/gcc_64/bin/qt-cmake \
   QT_HOST_PATH=/home/leonamramosfoli/qt-cli/6.5.3/gcc_64 \
   tools/android/build.sh`

Result:
- CMake failed: Qt6Charts not found in host Qt.
- Missing config:
  - `/home/leonamramosfoli/qt-cli/6.5.3/gcc_64/lib/cmake/Qt6Charts/Qt6ChartsConfig.cmake`

Blocker:
- Qt6 Charts module missing in qt-cli install (host + likely android). Build cannot continue.

## 9) Build failure: missing Lame
Error:
- `Could NOT find Lame (missing: LAME_INCLUDE_DIRS LAME_LIBRARIES)`

Attempted fix:
- `sudo apt install -y libmp3lame-dev`

Result:
- sudo requires password (TTY). Cannot proceed automatically in this environment.

## 10) Install Qt modules via aqtinstall (online)
Commands:
- `python3 -m venv ~/.venvs/aqt && ~/.venvs/aqt/bin/pip install -U pip aqtinstall`
- `~/.venvs/aqt/bin/aqt install-qt linux desktop 6.5.3 gcc_64 -m qtcharts qtmultimedia --outputdir ~/Qt`
- `~/.venvs/aqt/bin/aqt install-qt linux android 6.5.3 android_arm64_v8a -m qtcharts qtmultimedia --outputdir ~/Qt`

Result:
- Qt Charts + Multimedia installed for host and Android arm64.

## 11) Build APK (arm64-v8a)
Command:
- `QT_ANDROID_PREFIX_ARM64=/home/leonamramosfoli/Qt/6.5.3/android_arm64_v8a \
   QT_CMAKE_BIN=/home/leonamramosfoli/Qt/6.5.3/gcc_64/bin/qt-cmake \
   QT_HOST_PATH=/home/leonamramosfoli/Qt/6.5.3/gcc_64 \
   tools/android/build.sh`

Result:
- Build succeeded.
- APK created: `dist/welle-io-arm64-v8a.apk` (unsigned)

Notes:
- CMake used cached Qt paths under `/home/leonamramosfoli/qt-cli/...` in the build dir.

## 12) APK validation (unsigned)
Commands:
- `apksigner verify --verbose --print-certs dist/welle-io-arm64-v8a.apk`
- `aapt dump badging dist/welle-io-arm64-v8a.apk`

Results:
- `apksigner`: `DOES NOT VERIFY` / `ERROR: Missing META-INF/MANIFEST.MF`
- `aapt` shows `native-code: 'arm64-v8a'` and targetSdkVersion 31.

## 13) Install attempt (unsigned)
Command:
- `adb install -r dist/welle-io-arm64-v8a.apk`

Result:
- Failure: `INSTALL_PARSE_FAILED_NO_CERTIFICATES` (matches issue #814 symptoms)

## 14) Sign APK with debug keystore
Command:
- `apksigner sign --ks ~/.android/debug.keystore --ks-pass pass:android --key-pass pass:android --ks-key-alias androiddebugkey --out dist/welle-io-arm64-v8a-signed.apk dist/welle-io-arm64-v8a.apk`

Result:
- Signed APK verifies with v1/v2/v3 schemes.

## 15) Install signed APK
Command:
- `adb install -r dist/welle-io-arm64-v8a-signed.apk`

Result:
- Success (after a harmless "incremental install not allowed" warning).

## 16) Fix applied in build script
Change:
- `tools/android/build.sh` now copies unsigned APK to `dist/welle-io-arm64-v8a-unsigned.apk` and, if debug keystore is present, signs to `dist/welle-io-arm64-v8a.apk` automatically.
- Also fixed `rg --no-daemon` usage to avoid ripgrep flag parsing errors.
