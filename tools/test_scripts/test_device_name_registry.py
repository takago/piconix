#!/usr/bin/env python3
"""Canonical device names are shared by kernel VFS and trace viewer."""
from pathlib import Path
import subprocess
root = Path(__file__).resolve().parents[2]
h = (root/'include/device_names.h').read_text()
k = (root/'kernel/kernel.c').read_text()
t = (root/'userland/apps/trace.c').read_text()
for name, ident in [('console','DEV_CONSOLE'),('ttyS1','DEV_UART1'),('ttyN0','DEV_WIFI_TTY'),('net/wifi','DEV_WIFI_CTRL'),('trace','DEV_TRACE')]:
    assert f'"/dev/{name}", {ident}' in h
assert 'PICONIX_FIXED_DEVICES(DEVICE_ENTRY)' in k
assert 'PICONIX_WIFI_DEVICES(DEVICE_ENTRY)' in k
assert 'PICONIX_FIXED_DEVICES(DEVICE_CASE)' in t
assert 'PICONIX_WIFI_DEVICES(DEVICE_CASE)' in t
assert 'TRACE_TTY(e->kind)' in t
assert 'program_name(e->program_id)' in t
subprocess.run(['cc','-std=c11','-fsyntax-only','-I',str(root/'userland/lib'),'-I',str(root/'include'),str(root/'userland/apps/trace.c')],check=True)
print('PASS: shared device registry and trace TTY decoding')
