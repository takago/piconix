#!/usr/bin/env python3
"""Unit tests for Stage 8 readelf parser and overlap checks (no SDK needed)."""
from audit_wifi_elf import parse_sections, audit
sample = '''  [ 1] .data PROGBITS 20001000 001000 000200 00 WA 0 0 4
  [ 2] .bss NOBITS 20002000 001200 001000 00 WA 0 0 4
  [ 3] .text PROGBITS 10000000 000100 003000 00 AX 0 0 4
'''
sections = list(parse_sections(sample))
assert len(sections) == 2, sections
errors, size = audit(sections)
assert not errors and size == 0x1200, (errors, size)
errors, _ = audit(sections + [('.bad', 0x20038000, 128)])
assert errors and '.bad' in errors[0]
errors, _ = audit(sections + [('.overflow', 0x2000fff0, 32)])
assert errors and '.overflow' in errors[0]
print('PASS: ELF writable-section parser and reserved-RAM overlap rejection')
