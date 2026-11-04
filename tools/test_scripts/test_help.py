#!/usr/bin/env python3
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
main = (root/'kernel/main.c').read_text()
help_src = (root/'userland/apps/help.c').read_text()
shell = (root/'userland/apps/shell.c').read_text()
cmake = (root/'CMakeLists.txt').read_text()
names = re.findall(r'register_image\(\d+,"([a-z_0-9]+)"', main)
assert names and len(names) == len(set(names)), "duplicate or missing registered programs"
build_match = re.search(r'foreach\(APP IN ITEMS ([^)]+)\)', cmake)
assert build_match, "missing app build list"
built = set(build_match.group(1).split())
registered_sources = {'shell' if name == 'sh' else 'asm' if name == 'as' else name for name in names}
assert built == registered_sources, f"build/registration drift: {built ^ registered_sources}"
for name in names + ['cd','getty','exit','logout']:
    assert f'!strcmp(argv[1],"{name}")' in help_src, name
    assert f'  {name}' in help_src, name
assert 'if(eq(s,"help"))' not in shell
assert 'gpio fbed asm help edit vi dd mem tree find nc httpd tcp_echod udp_echod)' in cmake
# Help text must describe real options and the documented user-space build flags.
for syntax in ('cat [-n] [file]', 'ls [-la] [path]', 'edit /tmp/FILE',
               'dd [if=FILE] [of=FILE]', 'grep PATTERN [file]'):
    assert syntax in help_src, f'missing command usage: {syntax}'
assert 'set(APP_OPT -Oz)' in cmake
assert '-fPIC' in cmake
print('PASS: help coverage, shell delegation, and CMake registration')
