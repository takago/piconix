#!/usr/bin/env python3
"""Fail closed unless a single RAM image works at either slot origin.

Checks binary equality, absence of unresolved or dynamic relocations, and
forbidden runtime sections. Validity does not substitute for hardware testing.
"""
import argparse
import pathlib
import re
import subprocess

p=argparse.ArgumentParser()
p.add_argument('elf')
p.add_argument('binary')
p.add_argument('alt_elf')
p.add_argument('alt_binary')
p.add_argument('--readelf',default='readelf')
a=p.parse_args()
assert pathlib.Path(a.binary).read_bytes()==pathlib.Path(a.alt_binary).read_bytes(), 'PIC image changes with link address'
for elf, origin in ((a.elf, 0x20010000), (a.alt_elf, 0x20018000)):
    sect=subprocess.check_output([a.readelf,'-SW',elf],text=True)
    for forbidden in ('.got', '.dynamic', '.plt'):
        assert forbidden not in sect, f'{elf}: unsupported {forbidden}'
    rel=subprocess.check_output([a.readelf,'-rW',elf],text=True)
    assert 'There are no relocations in this file.' in rel, f'{elf}: relocations remain'
    syms=subprocess.check_output([a.readelf,'-sW',elf],text=True)
    assert not any(' UND ' in line and line.split()[-1]!='UND' for line in syms.splitlines()), f'{elf}: unresolved symbols'
    entries = re.findall(r'^\s*\d+:\s*([0-9a-fA-F]+)\s+\d+\s+FUNC\s+GLOBAL\s+DEFAULT\s+\d+\s+app_entry\s*$',syms,re.M)
    assert len(entries)==1 and (int(entries[0],16)&~1)==origin, f'{elf}: PIC entry is not at slot origin'
print('PASS: PIC images identical at two RAM addresses; no relocations/GOT')
