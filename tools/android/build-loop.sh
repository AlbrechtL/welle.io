#!/usr/bin/env bash
set -euo pipefail

QT_VER="${QT_VER:-6.5.3}"
QT_ANDROID_PREFIX_ARM64="${QT_ANDROID_PREFIX_ARM64:-$HOME/Qt/$QT_VER/android_arm64_v8a}"
QT_HOST_PATH="${QT_HOST_PATH:-$HOME/Qt/$QT_VER/gcc_64}"
QT_CMAKE_BIN="${QT_CMAKE_BIN:-$QT_HOST_PATH/bin/qt-cmake}"

MAX_ITERS=6
ITER=0

while true; do
  ITER=$((ITER+1))
  echo "== Build attempt $ITER =="
  set +e
  QT_ANDROID_PREFIX_ARM64="$QT_ANDROID_PREFIX_ARM64" \
  QT_HOST_PATH="$QT_HOST_PATH" \
  QT_CMAKE_BIN="$QT_CMAKE_BIN" \
  tools/android/build.sh 2>&1 | tee /tmp/welle-android-build.log
  RC=${PIPESTATUS[0]}
  set -e
  if [[ $RC -eq 0 ]]; then
    echo "Build succeeded."
    exit 0
  fi

  # Detect missing Qt component
  MISSING=$(rg -o "Failed to find required Qt component \"([A-Za-z0-9_]+)\"" /tmp/welle-android-build.log | head -n1 | sed -E 's/.*\"(.*)\"/\1/')
  if [[ -z "$MISSING" ]]; then
    # Alternate form: Could NOT find Qt6Multimedia (missing: Qt6Multimedia_DIR)
    MISSING=$(rg -o "Could NOT find (Qt6[A-Za-z0-9_]+)" /tmp/welle-android-build.log | head -n1 | sed -E 's/Could NOT find (Qt6[A-Za-z0-9_]+)/\1/')
  fi

  if [[ -z "$MISSING" ]]; then
    echo "Build failed but missing Qt module not detected. See /tmp/welle-android-build.log" >&2
    exit 1
  fi

  echo "Missing Qt module detected: $MISSING"
  # Strip Qt6 prefix for aqt module name if present
  MODULE="$MISSING"
  MODULE=${MODULE#Qt6}

  tools/android/install-qt-module.sh "$MODULE"

  if [[ $ITER -ge $MAX_ITERS ]]; then
    echo "Max iterations reached" >&2
    exit 1
  fi

done
