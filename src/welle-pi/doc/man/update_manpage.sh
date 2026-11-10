#!/bin/sh

help2man \
  --name "welle.io interface for the Raspberry Pi" \
  --no-info \
  --no-discard-stderr \
  --help-option='-h' \
  --version-option="-v" \
  --output welle-pi.1 \
  welle-pi
