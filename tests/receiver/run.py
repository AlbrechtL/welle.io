#!/usr/bin/env python3
"""Deterministic receiver regressions; requires g++, Python 3 and Node.js."""
from pathlib import Path
import subprocess, tempfile, sys
root=Path(__file__).resolve().parents[2]
tests=root/'tests/receiver'
def run(args): subprocess.run(list(map(str,args)),cwd=root,check=True,timeout=180)
with tempfile.TemporaryDirectory(prefix='welle-receiver-') as directory:
 out=Path(directory)
 for name in ['test-iq-flow','test-iq-stats','test-pacer','test-counter']:
  run(['g++','-std=c++14','-O1','-pthread','-fsanitize=undefined','-I'+str(root),'-I'+str(root/'src/input'),'-I'+str(root/'src/various'),tests/(name+'.cpp'),'-o',out/name])
  run([out/name]); print('PASS:',name,flush=True)
 run([sys.executable,tests/'test-frame.py',root,out/'frame'])
 run([sys.executable,tests/'test-agc.py'])
 for name in ['test-rate','test-iq-graph','test-iq-mean','test-iq-scale']:
  run(['node',tests/(name+'.cjs')])
 for name in ['player-service-id.js','channel-ui.js']:
  run(['node',root/'tests/stability'/name])
