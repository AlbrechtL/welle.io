#!/usr/bin/env python3
"""Link the real decoder against an existing CLI build and stress shutdown."""
from pathlib import Path
import sys,shlex,subprocess,tempfile
root=Path(__file__).resolve().parents[2]
build=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='welle-stop-test-') as directory:
    temp=Path(directory); obj=temp/'stop.o'; binary=temp/'stop'
    subprocess.run(['g++','-std=c++14','-O1','-g','-pthread', '-I'+str(root/'src'),'-I'+str(root/'src/backend'),'-I'+str(root/'src/various'),'-c',str(root/'tests/stability/decoder-stop.cpp'),'-o',str(obj)],check=True,timeout=60)
    command=shlex.split((build/'CMakeFiles/welle-cli.dir/link.txt').read_text())
    old=next(a for a in command if a.endswith('/welle-cli.cpp.o'))
    command[command.index(old)]=str(obj)
    command[command.index('-o')+1]=str(binary)
    subprocess.run(command,cwd=build,check=True,timeout=60)
    subprocess.run([str(binary)],check=True,timeout=45)
