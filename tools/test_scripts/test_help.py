#!/usr/bin/env python3
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
main = (root/'kernel/main.c').read_text()
help_src = (root/'userland/help.c').read_text()
shell = (root/'userland/shell.c').read_text()
cmake = (root/'CMakeLists.txt').read_text()
names = re.findall(r'register_image\(\d+,"([a-z]+)"', main)
assert len(names) == 35 and len(set(names)) == 35
for name in names + ['cd','getty','exit','logout']:
    assert f'!strcmp(argv[1],"{name}")' in help_src, name
    assert f'  {name}' in help_src, name
assert 'if(eq(s,"help"))' not in shell
assert 'gpio fbed asm help edit mem tree find nc httpd tcp_echod udp_echod)' in cmake
print('PASS: help coverage, shell delegation, and CMake registration')
