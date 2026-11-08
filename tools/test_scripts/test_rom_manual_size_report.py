#!/usr/bin/env python3
"""Ensure XIP manuals have their own non-overlapping firmware size category."""
import importlib.util
from pathlib import Path
from tempfile import TemporaryDirectory

root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('size_ram', root / 'tools/size_ram.py')
size_ram = importlib.util.module_from_spec(spec)
spec.loader.exec_module(size_ram)

assert 'ROM manual' in size_ram.CATEGORIES
assert size_ram.component('/build/CMakeFiles/piconix.dir/embed_manual.S.obj') == 'ROM manual'
assert size_ram.component('/build/CMakeFiles/piconix.dir/kernel/kernel.c.obj') == 'PicoNIX'

base = 0x10000000
sections = [('.text', base, 32), ('.manual', base + 32, 24),
            ('.data', 0x20000000, 8)]
manual = size_ram.manual_ranges(sections)
assert manual == [(base + 32, base + 56, 'Flash', 'ROM manual')]
assert size_ram.manual_ranges([('.manual', 0x20000000, 24)]) == []
with TemporaryDirectory() as d:
    path = Path(d) / 'test.map'
    path.write_text('''.text           0x10000000 0x00000020
 .text           0x10000000 0x00000020 /src/kernel/kernel.c.o
.manual         0x10000020 0x00000018
 .manual         0x10000020 0x00000018 /build/CMakeFiles/piconix.dir/embed_manual.S.obj
.data           0x20000000 0x00000008
 .data           0x20000000 0x00000008 /src/kernel/kernel.c.o
''')
    # Even an overlapping app-symbol span cannot steal manual bytes.
    embedded = [(base + 16, base + 40, 'Flash', 'Embedded apps')]
    buckets = size_ram.map_report(path, sections, embedded, manual)
    assert buckets['ROM manual', 'Flash'] == 24, buckets
    assert buckets['Embedded apps', 'Flash'] == 16, buckets
    assert buckets['PicoNIX', 'Flash'] == 16, buckets
    assert buckets['PicoNIX', 'SRAM'] == 8, buckets
    assert sum(buckets[c, 'Flash'] for c in size_ram.CATEGORIES) == 56, buckets
    assert sum(buckets[c, 'SRAM'] for c in size_ram.CATEGORIES) == 8, buckets

print('PASS: ROM manual section isolated, Flash/SRAM totals preserved, overlaps deduplicated')
