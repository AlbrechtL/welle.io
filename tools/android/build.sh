#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DIST_DIR="$ROOT_DIR/dist"
BUILD_DIR="$ROOT_DIR/build-android-arm64"

ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-${ANDROID_HOME:-$HOME/Android/Sdk}}"
if [[ ! -d "$ANDROID_SDK_ROOT" ]]; then
  echo "ANDROID_SDK_ROOT not found at $ANDROID_SDK_ROOT" >&2
  exit 1
fi

# Pick newest build-tools directory
BUILD_TOOLS_DIR="$(ls -1d "$ANDROID_SDK_ROOT"/build-tools/* 2>/dev/null | sort -V | tail -n1 || true)"
if [[ -z "$BUILD_TOOLS_DIR" ]]; then
  echo "No build-tools found under $ANDROID_SDK_ROOT/build-tools" >&2
  exit 1
fi

export PATH="$ANDROID_SDK_ROOT/platform-tools:$ANDROID_SDK_ROOT/cmdline-tools/latest/bin:$BUILD_TOOLS_DIR:$PATH"

# Prefer NDK r25b, otherwise newest installed
NDK_DIR=""
if [[ -d "$ANDROID_SDK_ROOT/ndk/25.1.8937393" ]]; then
  NDK_DIR="$ANDROID_SDK_ROOT/ndk/25.1.8937393"
else
  NDK_DIR="$(ls -1d "$ANDROID_SDK_ROOT"/ndk/* 2>/dev/null | sort -V | tail -n1 || true)"
fi
if [[ -z "$NDK_DIR" ]]; then
  echo "No NDK found under $ANDROID_SDK_ROOT/ndk" >&2
  exit 1
fi

# Qt Android path must be provided by user or preinstalled
QT_CMAKE_BIN="${QT_CMAKE_BIN:-}"  # optional explicit path to qt-cmake
QT_ANDROID_PREFIX_ARM64="${QT_ANDROID_PREFIX_ARM64:-}" # e.g. /opt/Qt/6.5.3/android_arm64_v8a
QT_HOST_PATH="${QT_HOST_PATH:-}"  # optional

if [[ -z "$QT_CMAKE_BIN" ]]; then
  if command -v qt-cmake >/dev/null 2>&1; then
    QT_CMAKE_BIN="$(command -v qt-cmake)"
  elif [[ -x /usr/lib/qt6/bin/qt-cmake ]]; then
    QT_CMAKE_BIN="/usr/lib/qt6/bin/qt-cmake"
  fi
fi

if [[ -z "$QT_CMAKE_BIN" || ! -x "$QT_CMAKE_BIN" ]]; then
  echo "qt-cmake not found. Set QT_CMAKE_BIN or ensure Qt host tools are installed." >&2
  exit 1
fi

if [[ -z "$QT_ANDROID_PREFIX_ARM64" || ! -d "$QT_ANDROID_PREFIX_ARM64" ]]; then
  echo "Qt Android arm64 prefix not found. Set QT_ANDROID_PREFIX_ARM64 (e.g. /opt/Qt/6.5.3/android_arm64_v8a)." >&2
  exit 1
fi

QT_TOOLCHAIN_FILE="$QT_ANDROID_PREFIX_ARM64/lib/cmake/Qt6/qt.toolchain.cmake"
if [[ ! -f "$QT_TOOLCHAIN_FILE" ]]; then
  echo "Qt toolchain file not found at $QT_TOOLCHAIN_FILE" >&2
  exit 1
fi

mkdir -p "$BUILD_DIR" "$DIST_DIR"

ARGS=(
  -DCMAKE_TOOLCHAIN_FILE="$QT_TOOLCHAIN_FILE"
  -DQT_HOST_PATH="$QT_HOST_PATH"
  -DANDROID_SDK_ROOT="$ANDROID_SDK_ROOT"
  -DANDROID_NDK_ROOT="$NDK_DIR"
  -DANDROID_ABI=arm64-v8a
  -DANDROID_PLATFORM=android-34
  -DCMAKE_POLICY_DEFAULT_CMP0057=NEW
  -DQT_ENABLE_VERBOSE_DEPLOYMENT=ON
  -DCMAKE_VERBOSE_MAKEFILE=ON
)

"$QT_CMAKE_BIN" "${ARGS[@]}" -S "$ROOT_DIR" -B "$BUILD_DIR"

# If offline, force Gradle wrapper to use a locally cached distribution.
LOCAL_GRADLE_ZIP=""
LOCAL_GRADLE_ZIP=$(ls -1 "$HOME/.gradle/wrapper/dists/gradle-8.7-bin"/*/gradle-8.7-bin.zip 2>/dev/null | head -n1 || true)
if [[ -z "$LOCAL_GRADLE_ZIP" ]]; then
  LOCAL_GRADLE_ZIP=$(ls -1 "$HOME/.gradle/wrapper/dists/gradle-8.13-bin"/*/gradle-8.13-bin.zip 2>/dev/null | head -n1 || true)
fi

if [[ -n "$LOCAL_GRADLE_ZIP" ]]; then
  TEMPLATE_WRAPPER="$QT_ANDROID_PREFIX_ARM64/src/3rdparty/gradle/gradle/wrapper/gradle-wrapper.properties"
  if [[ -f "$TEMPLATE_WRAPPER" ]]; then
    sed -i "s|^distributionUrl=.*|distributionUrl=file://$LOCAL_GRADLE_ZIP|" "$TEMPLATE_WRAPPER"
  fi
  WRAPPER_PROPS="$BUILD_DIR/android-build/gradle/wrapper/gradle-wrapper.properties"
  if [[ -f "$WRAPPER_PROPS" ]]; then
    sed -i "s|^distributionUrl=.*|distributionUrl=file://$LOCAL_GRADLE_ZIP|" "$WRAPPER_PROPS"
  fi
fi

# Work around Gradle networking/wildcard issues in restricted environments.
GRADLE_PROPS_TEMPLATE="$QT_ANDROID_PREFIX_ARM64/src/3rdparty/gradle/gradle.properties"
if [[ -f "$GRADLE_PROPS_TEMPLATE" ]]; then
  if ! rg -q "^org.gradle.jvmargs=.*preferIPv4Stack" "$GRADLE_PROPS_TEMPLATE"; then
    printf '\norg.gradle.jvmargs=-Djava.net.preferIPv4Stack=true -Djava.net.preferIPv6Addresses=false\n' >> "$GRADLE_PROPS_TEMPLATE"
  fi
  if ! rg -q "^org.gradle.daemon=false" "$GRADLE_PROPS_TEMPLATE"; then
    printf '\norg.gradle.daemon=false\n' >> "$GRADLE_PROPS_TEMPLATE"
  fi
fi

# Force Gradle wrapper to run without daemon (avoid socket bind failures).
GRADLEW_TEMPLATE="$QT_ANDROID_PREFIX_ARM64/src/3rdparty/gradle/gradlew"
if [[ -f "$GRADLEW_TEMPLATE" ]]; then
  if ! rg -q -- "--no-daemon" "$GRADLEW_TEMPLATE"; then
    sed -i 's/"\$@"/"\$@" --no-daemon/' "$GRADLEW_TEMPLATE"
  fi
fi
GRADLEW_BUILD="$BUILD_DIR/android-build/gradlew"
if [[ -f "$GRADLEW_BUILD" ]]; then
  if ! rg -q -- "--no-daemon" "$GRADLEW_BUILD"; then
    sed -i 's/"\$@"/"\$@" --no-daemon/' "$GRADLEW_BUILD"
  fi
fi
GRADLE_PROPS_BUILD="$BUILD_DIR/android-build/gradle.properties"
if [[ -f "$GRADLE_PROPS_BUILD" ]]; then
  if ! rg -q "^org.gradle.jvmargs=.*preferIPv4Stack" "$GRADLE_PROPS_BUILD"; then
    printf '\norg.gradle.jvmargs=-Djava.net.preferIPv4Stack=true -Djava.net.preferIPv6Addresses=false\n' >> "$GRADLE_PROPS_BUILD"
  fi
  if ! rg -q "^org.gradle.daemon=false" "$GRADLE_PROPS_BUILD"; then
    printf '\norg.gradle.daemon=false\n' >> "$GRADLE_PROPS_BUILD"
  fi
fi

# Build APK target (Qt Android)
cmake --build "$BUILD_DIR" --parallel --target apk

APK_PATH=""
APK_PATH="$(find "$BUILD_DIR" -path "*/android-build/build/outputs/apk/*/*.apk" | head -n1 || true)"
if [[ -z "$APK_PATH" ]]; then
  echo "APK not found under $BUILD_DIR" >&2
  exit 1
fi

UNSIGNED_APK="$DIST_DIR/welle-io-arm64-v8a-unsigned.apk"
SIGNED_APK="$DIST_DIR/welle-io-arm64-v8a.apk"
cp -f "$APK_PATH" "$UNSIGNED_APK"

# If a debug keystore is available, sign for local install convenience.
if command -v apksigner >/dev/null 2>&1 && [[ -f "$HOME/.android/debug.keystore" ]]; then
  apksigner sign \
    --ks "$HOME/.android/debug.keystore" \
    --ks-pass pass:android \
    --key-pass pass:android \
    --ks-key-alias androiddebugkey \
    --out "$SIGNED_APK" \
    "$UNSIGNED_APK"
  echo "APK (signed): $SIGNED_APK"
else
  cp -f "$UNSIGNED_APK" "$SIGNED_APK"
  echo "APK (unsigned): $SIGNED_APK"
fi
