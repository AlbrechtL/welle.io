#!/bin/sh

help2man \
  --name "welle.io command line interface" \
  --no-info \
  --no-discard-stderr \
  --help-option='-h' \
  --version-option="-v" \
  --output welle-cli.1 \
  welle-cli
