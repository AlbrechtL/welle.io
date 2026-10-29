#!/usr/bin/env python3
"""Compile actual production functions and exercise local bounded regressions."""
from pathlib import Path
import subprocess, tempfile, os
root=Path(__file__).resolve().parents[2]
source=(root/'src/welle-cli/webradiointerface.cpp').read_text()
start=source.index('static int remaining_ms(')
end=source.index('\nbool WebRadioInterface::dispatch_client',start)
with tempfile.TemporaryDirectory(prefix='welle-stability-') as temporary:
    temp=Path(temporary)
    (temp/'http-parser.inc').write_text(source[start:end])
    command=['g++','-std=c++14','-O1','-g','-pthread','-fsanitize='+os.environ.get('WELLE_SANITIZERS','address,undefined'),'-fno-omit-frame-pointer',
       '-I'+str(temp),'-I'+str(root/'src'),'-I'+str(root/'src/backend'),'-I'+str(root/'src/various'),
       str(root/'tests/stability/check.cpp'),str(root/'src/various/Socket.cpp'),
       str(root/'src/welle-cli/webprogrammehandler.cpp'),str(root/'src/backend/mot_manager.cpp'),
       str(root/'src/backend/charsets.cpp'),str(root/'src/backend/tools.cpp'),'-lmp3lame','-o',str(temp/'check')]
    if os.environ.get('WELLE_FLAC_TEST') == '1': command += ['-DHAVE_FLAC','-lFLAC++','-lFLAC']
    subprocess.run(command,check=True,timeout=120)
    subprocess.run([str(temp/'check')],check=True,timeout=30,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=1'})
