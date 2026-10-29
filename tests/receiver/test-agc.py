#!/usr/bin/env python3
from pathlib import Path
import subprocess, tempfile
root=Path(__file__).resolve().parents[2]
s=(root/'src/input/soapy_sdr.cpp').read_text()
a=s.index('namespace { // AGC helpers');b=s.index('} // AGC helpers',a)+len('} // AGC helpers')
with tempfile.TemporaryDirectory(prefix='welle-agc-') as name:
 p=Path(name);(p/'test.cpp').write_text('#include <array>\n#include <vector>\n#include <string>\n#include <cmath>\n#include <algorithm>\n#include <cassert>\n#include <iostream>\nusing std::size_t;\n'+s[a:b]+(root/'tests/receiver/agc-main.cpp').read_text())
 subprocess.run(['g++','-std=c++14','-Wall','-Wextra','-Werror','-fsanitize=undefined',str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
