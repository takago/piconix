#!/usr/bin/env python3
"""Build a compact, line-oriented, read-only symbol catalog from RAM app ELFs.

Output format: app<TAB>hex_offset<TAB>type<TAB>symbol<LF>.
The file is embedded in firmware Flash and served via /proc/syms.
"""
import argparse
import re
import subprocess
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--nm', required=True)
p.add_argument('--output', required=True)
p.add_argument('elf', nargs='+')
a = p.parse_args()
lines = []
for elf_file in a.elf:
    elf = Path(elf_file)
    app = elf.stem
    if app == 'shell': app = 'sh'
    if app == 'asm': app = 'as'
    try:
        result = subprocess.run([a.nm, '-n', '--defined-only', str(elf)], capture_output=True, text=True)
    except OSError:
        continue
    if result.returncode != 0:
        # シンボル表がないアプリは収録を省略する．ファームウェア生成は継続する．
        continue
    for row in result.stdout.splitlines():
        match = re.fullmatch(r'([0-9a-fA-F]+)\s+([TtDdBbRr])\s+([A-Za-z_.$][A-Za-z_0-9.$]*)', row.strip())
        if match is None: continue
        address, kind, name = match.groups()
        address = int(address, 16)
        if not 0x2000E000 <= address < 0x20014000: continue
        if name.startswith('__') or name.startswith('$'): continue
        lines.append((app, address - 0x2000E000, kind, name))
lines.sort(key=lambda x: (x[0], x[1], x[3]))
raw = ''.join(f'{app}\t{offset:04x}\t{kind}\t{name}\n' for app, offset, kind, name in lines).encode('ascii')
# 出力が同一なら書き換えず，再リンクを避ける．
output = Path(a.output)
if not output.exists() or output.read_bytes() != raw:
    output.write_bytes(raw)
print(f'PicoNIX: embedded {len(lines)} userland symbols ({len(raw)} bytes)')
