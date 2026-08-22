#!/usr/bin/env bash
set -euo pipefail

QT_VER="${QT_VER:-6.5.3}"
MODULE="$1"
if [[ -z "$MODULE" ]]; then
  echo "Usage: $0 <QtModuleName>" >&2
  exit 1
fi

# Ensure aqt is available via local venv
VENV="$HOME/.venvs/aqt"
if [[ ! -x "$VENV/bin/python" ]]; then
  python3 -m venv "$VENV"
fi

# Install aqtinstall if missing
if ! "$VENV/bin/python" -c 'import aqt' >/dev/null 2>&1; then
  "$VENV/bin/pip" install -U pip aqtinstall
fi

AQT="$VENV/bin/aqt"

"$AQT" install-qt linux desktop "$QT_VER" gcc_64 -m "$MODULE" --outputdir "$HOME/Qt"
"$AQT" install-qt linux android "$QT_VER" android_arm64_v8a -m "$MODULE" --outputdir "$HOME/Qt"
