#!/usr/bin/env python3
"""Static contract checks, not executable Pico W or parser runtime tests."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
manual=root/'manual'
required={'README','agent','workflows','programming','limits','languages/c','languages/asm','languages/python','c-examples','asm-examples','python-examples','memory','trace','devices','network','syscalls','troubleshooting'}
names={p.relative_to(manual).as_posix() for p in manual.rglob('*') if p.is_file()}
assert required <= names
assert len(names)>=38
index=(manual/'README').read_text()
assert all(n in index for n in names if n!='README')
for name in names:
    data=(manual/name).read_bytes()
    assert 0<len(data)<4096,(name,len(data))
    assert b'\x00' not in data
c=(manual/'languages/c').read_text()
a=(manual/'languages/asm').read_text()
p=(manual/'languages/python').read_text()
for value in ['1024','4096','16 functions','4 parameters','256','64']:
    assert value in c,value
for value in ['2048','2040','32 labels','svc #0','.align 2']:
    assert value in a,value
for value in ['2048','64 nonempty lines','16','256','socket(2)']:
    assert value in p,value
assert 'socket(SOCK_DGRAM)' not in (manual/'python-examples').read_text()
assert 'SOCK_STREAM/DGRAM names are NOT built-in' in p
print(f'PASS: {len(names)} pages present, indexed, bounded, and basic source-derived limits documented')
