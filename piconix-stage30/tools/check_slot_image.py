#!/usr/bin/env python3
"""Validate ELF loadable segments for a fixed RP2040 RAM task slot."""
import argparse
import pathlib
import re
import subprocess
import sys

p=argparse.ArgumentParser()
p.add_argument('elf')
p.add_argument('binary')
p.add_argument('origin',type=lambda s:int(s,0))
p.add_argument('--readelf',default='readelf')
a=p.parse_args()
end=a.origin+65536
out=subprocess.check_output([a.readelf,'-lW',a.elf],text=True)
segments=[]
for line in out.splitlines():
    m=re.match(r'\s*LOAD\s+0x[\da-fA-F]+\s+(0x[\da-fA-F]+)\s+0x[\da-fA-F]+\s+(0x[\da-fA-F]+)\s+(0x[\da-fA-F]+)',line)
    if m:
        start,filesz,memsz=(int(x,16) for x in m.groups())
        if memsz:
            assert a.origin <= start and start+memsz <= end, f'out-of-slot LOAD: {line}'
            segments.append((start,filesz,memsz))
assert segments,'No LOAD segments found'
assert min(s for s,_,_ in segments)==a.origin,'Image must start at slot origin'
image_end=max(s+f for s,f,_ in segments)
size=pathlib.Path(a.binary).stat().st_size
assert size==image_end-a.origin,(size,image_end-a.origin)
assert image_end<=end-4096,'Image overlaps 4 KiB stack reservation'
sym=subprocess.check_output([a.readelf,'-sW',a.elf],text=True)
entries=re.findall(r'^\s*\d+:\s*([0-9a-fA-F]+)\s+\d+\s+FUNC\s+GLOBAL\s+DEFAULT\s+\d+\s+app_entry\s*$',sym,re.M)
assert len(entries)==1,'Missing or duplicate app_entry symbol'
entry=int(entries[0],16)&~1
assert a.origin<=entry<end-4096,'app_entry outside task code area'
print(f'OK: slot=0x{a.origin:08x}, file={size} bytes, stack reserved=4096 bytes')
