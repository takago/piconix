#!/usr/bin/env python3
"""Structural regressions for interactive goto, RAM labels and new commands."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
k=(r/'kernel/kernel.c').read_text()
m=(r/'userland/mem.c').read_text()
main=(r/'kernel/main.c').read_text()
cmake=(r/'CMakeLists.txt').read_text()
assert 'DEV_PROC_RAMFILES' in k
assert 'tmp_files[i].data' in k
assert 'proc_ramfiles_read' in k
assert 'RAM_FILE: /tmp/%s' in m
assert "if(c=='g'||c=='G')" in m
assert 'refresh_ram_labels(addr);' in m
assert 'tree find)' in cmake
for name in ('tree','find'):
 assert f'register_image({32 if name=="tree" else 33},"{name}"' in main
 assert (r/'userland'/f'{name}.c').is_file()
assert (r/'userland/walk.h').is_file()
print('PASS: mem goto/RAM metadata and tree/find registration')
