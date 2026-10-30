#!/usr/bin/env python3
"""Update a fixed-size embedded kernel symbol catalog from a linked ELF.

Its constant section size makes the second link address-stable. The first four
bytes store the actual text length; the rest is zero padding, never exposed by
/proc/syms. No output rewrite if content is unchanged.
"""
import argparse
from pathlib import Path
import re
import struct
import subprocess
import sys

CAPACITY = 32768
p = argparse.ArgumentParser()
p.add_argument('--nm', required=True)
p.add_argument('--elf', required=True)
p.add_argument('--output', required=True)
p.add_argument('--verify-only', action='store_true')
a = p.parse_args()
result = subprocess.run([a.nm, '-n', '--defined-only', a.elf], text=True, capture_output=True)
if result.returncode:
    sys.exit(result.stderr or 'nm failed')
rows = []
for line in result.stdout.splitlines():
    m = re.fullmatch(r'([0-9a-fA-F]+)\s+([TtDdBbRr])\s+([A-Za-z_.$][A-Za-z_0-9.$]*)', line.strip())
    if not m:
        continue
    addr, kind, name = m.groups()
    addr = int(addr, 16)
    if not (0x10000000 <= addr < 0x10200000 or 0x20000000 <= addr < 0x20018000):
        continue
    if name.startswith(('__', '$')) or name.startswith('piconix_kernel_syms_'):
        continue
    rows.append(f'kernel\t{addr:08x}\t{kind}\t{name}\n')
raw = ''.join(rows).encode('ascii')
if len(raw) > CAPACITY - 4:
    sys.exit(f'kernel symbols need {len(raw)} bytes, reserved {CAPACITY - 4}')
data = struct.pack('<I', len(raw)) + raw + bytes(CAPACITY - 4 - len(raw))
output = Path(a.output)
if a.verify_only:
    if not output.exists() or output.read_bytes() != data:
        sys.exit('ERROR: kernel symbol addresses changed after final link')
    print(f'PASS: {len(rows)} kernel symbols, address-stable final link')
elif not output.exists() or output.read_bytes() != data:
    output.write_bytes(data)
    print(f'UPDATED: {len(rows)} kernel symbols, {len(raw)} bytes')
else:
    print(f'UNCHANGED: {len(rows)} kernel symbols')
