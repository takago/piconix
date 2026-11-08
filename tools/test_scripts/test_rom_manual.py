#!/usr/bin/env python3
"""Validate the binary XIP index, source pages and dedicated linker section."""
from pathlib import Path
import struct
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
files = sorted(p for p in (root / 'manual').rglob('*') if p.is_file())
assert len(files) >= 10
entry = struct.Struct('<32sII')
with tempfile.TemporaryDirectory() as temp:
    out = Path(temp) / 'manual.bin'
    subprocess.run(['python3', str(root / 'tools/make_manual_rom.py'), str(out)], check=True)
    original = out.read_bytes()
    subprocess.run(['python3', str(root / 'tools/make_manual_rom.py'), str(out)], check=True)
    assert original == out.read_bytes(), 'non-reproducible manual image'
assert original[:8] == b'PICONIXM'
count = struct.unpack_from('<I', original, 8)[0]
assert count == len(files)
assert len(original) >= 12 + count * entry.size
for i, page in enumerate(files):
    rawname, offset, size = entry.unpack_from(original, 12 + i * entry.size)
    name = rawname.split(b'\0', 1)[0].decode('ascii')
    assert name == page.relative_to(root / 'manual').as_posix()
    assert offset % 4 == 0 and offset >= 12 + count * entry.size
    assert original[offset:offset + size] == page.read_bytes()
    assert offset + size <= len(original)
kernel = (root / 'kernel/kernel.c').read_text()
for token in ('VDIR_MAN_LANG', 'MAN_BASE', 'man_entries[dev-MAN_BASE]', 'man_data(m)', '/man/'):
    assert token in kernel, token
header = (root / 'kernel/manual.h').read_text()
assert '__manual_start' in header and 'man_entry_t' in header
assert len(header) < 2000
assert 'KEEP(*(.manual))' in (root / 'kernel/section_extra_post_text.incl').read_text()
assert 'KEEP(*(.manual))' in (root / 'tools/limit_sdk_xip.py').read_text()
assert '.incbin "@PICONIX_MAN_BIN@"' in (root / 'cmake/embed_manual.S.in').read_text()
assert 'kernel/man_rom.h' not in (root / 'CMakeLists.txt').read_text()
print(f'PASS: {count} pages; {len(original)}-byte XIP image; index/data match; linker contracts')
