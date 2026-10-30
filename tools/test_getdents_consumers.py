#!/usr/bin/env python3
"""Ensure /bin consumers use getdents rather than obsolete text reads."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
for app in ('shell', 'watch', 'trace', 'mem'):
    source = (root / 'userland' / f'{app}.c').read_text()
    assert 'getdents(fd,' in source, app
    assert 'open("/bin"' in source, app
assert 'case 26: return "getdents";' in (root / 'userland/trace.c').read_text()
print('PASS: all /bin consumers use getdents; trace labels syscall 26')
