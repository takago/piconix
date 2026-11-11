#!/usr/bin/env python3
"""Static checks for reference examples in the ROM manual."""
from pathlib import Path
root=Path(__file__).resolve().parents[2]
man=root/'manual'
names=["example-c-hello","example-c-loops","example-c-functions","example-c-arrays","example-asm-hello","example-asm-stack","example-asm-pid","example-python-basics","example-python-arrays","example-python-files","example-python-temperature","example-python-udp","example-agent-verify"]
index=(man/'README').read_text()
for name in names:
    data=(man/name).read_bytes()
    assert 0<len(data)<4096 and b"\x00" not in data
    assert name in index,name
assert 'svc #0' in (man/'example-asm-hello').read_text()
assert 'cc /tmp/hello.c' in (man/'example-c-hello').read_text()
assert 'python /tmp/math.py' in (man/'example-python-basics').read_text()
assert 'ONLY WITH AUTHORIZATION' in (man/'example-python-udp').read_text()
print(f'PASS: {len(names)} example pages indexed, bounded, and structurally consistent')
