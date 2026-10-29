#!/usr/bin/env python3
"""Link the real HTTP router against a cached MOT fixture; no hardware access."""
from pathlib import Path
import sys,shlex,subprocess,tempfile
root=Path(__file__).resolve().parents[2]
build=Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='welle-stop-test-') as directory:
    temp=Path(directory); obj=temp/'stop.o'; binary=temp/'stop'
    subprocess.run(['g++','-std=c++14','-O1','-g','-pthread', '-I'+str(root/'src'),'-I'+str(root/'src/backend'),'-I'+str(root/'src/various'),'-I'+str(root/'src/input'),'-c',str(root/'tests/stability/slide-http.cpp'),'-o',str(obj)],check=True,timeout=60)
    command=shlex.split((build/'CMakeFiles/welle-cli.dir/link.txt').read_text())
    # Recent CMake adds a linker dependency output in the production build.
    # This test runs unprivileged and must keep all outputs in its tempdir.
    command=[('-Wl,--dependency-file='+str(temp/'link.d'))
             if arg.startswith('-Wl,--dependency-file=') else arg for arg in command]
    old=next(a for a in command if a.endswith('/welle-cli.cpp.o'))
    command[command.index(old)]=str(obj)
    command[command.index('-o')+1]=str(binary)
    subprocess.run(command,cwd=build,check=True,timeout=60)
    subprocess.run([str(binary)],check=True,timeout=20)
