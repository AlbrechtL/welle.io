#!/bin/sh
# Make build/welle-io.app self-contained: deploy its Qt and Homebrew
# libraries into the bundle and apply an ad-hoc signature.
# Used by .github/workflows/macos.yml and the README macOS instructions.
#
# Usage: scripts/macos-deploy.sh <qt-prefix> [app-bundle]
#   qt-prefix   Qt installation prefix, e.g. "$(brew --prefix qt)"
#   app-bundle  path to the app bundle (default: build/welle-io.app)
set -eu

qt_prefix=${1:?usage: $0 <qt-prefix> [app-bundle]}
app=${2:-build/welle-io.app}

"$qt_prefix/bin/macdeployqt" "$app" \
    -qmldir=src/welle-gui/QML \
    -libpath="$qt_prefix/lib" \
    -libpath="$(brew --prefix)/lib"
dylibbundler -b \
    -x "$app/Contents/MacOS/welle-io" \
    -d "$app/Contents/Frameworks" \
    -p @executable_path/../Frameworks \
    -cd -of -ns
codesign --force --deep --sign - "$app"
codesign --verify --deep --strict "$app"
if otool -L "$app/Contents/MacOS/welle-io" | grep -Eq '/(opt/homebrew|usr/local)/'; then
    echo "Application still links to Homebrew libraries" >&2
    exit 1
fi
