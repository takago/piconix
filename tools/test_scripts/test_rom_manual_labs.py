#!/usr/bin/env python3
"""Static ROM-lab checks; these do not run programs on a Pico."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
man=root/'manual'
names=["lab-c-hello","lab-c-loops","lab-c-functions","lab-c-arrays","lab-asm-hello","lab-asm-stack","lab-asm-pid","lab-python-basics","lab-python-arrays","lab-python-files","lab-python-temperature","lab-python-udp","lab-agent-verify"]
index=(man/'README').read_text()
for name in names:
    data=(man/name).read_bytes()
    assert 0<len(data)<4096 and b"\x00" not in data
    assert name in index,name
assert 'svc #0' in (man/'lab-asm-hello').read_text()
assert 'cc /tmp/hello.c' in (man/'lab-c-hello').read_text()
assert 'python /tmp/math.py' in (man/'lab-python-basics').read_text()
assert 'ONLY WITH AUTHORIZATION' in (man/'lab-python-udp').read_text()
print(f'PASS: {len(names)} lab pages indexed, bounded, and structurally consistent')
