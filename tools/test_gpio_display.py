#!/usr/bin/env python3
"""Check GPIO table formatting against the supported tiny printf syntax."""
from pathlib import Path
src = (Path(__file__).resolve().parents[1] / 'userland/gpio.c').read_text()
libc = (Path(__file__).resolve().parents[1] / 'userland/libc.c').read_text()
assert 'printf("%-18s",label)' in src
assert 'printf("%-*s"' not in src
assert "if(*fmt=='-')" in libc and "if(spec=='s')" in libc
print('PASS: GPIO fixed-width cells use supported printf format')
