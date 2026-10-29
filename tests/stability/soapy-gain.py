#!/usr/bin/env python3
"""Run the current range-aware AGC regression suite."""
import runpy
from pathlib import Path
runpy.run_path(str(Path(__file__).resolve().parents[1]/'receiver/test-agc.py'),run_name='__main__')
