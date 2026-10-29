#!/usr/bin/env python3
"""Verify that every registered RAM application ID fits PROGRAM_CAPACITY."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parent.parent
kernel = (ROOT / "kernel/kernel.c").read_text(encoding="utf-8")
main = (ROOT / "kernel/main.c").read_text(encoding="utf-8")
cap_match = re.search(r"^\s*#define\s+PROGRAM_CAPACITY\s+(\d+)\b", kernel, re.M)
if not cap_match:
    sys.exit("ERROR: PROGRAM_CAPACITY not found in kernel/kernel.c")
capacity = int(cap_match.group(1))
# Match only numeric IDs; fail if registration changes to a form we cannot validate.
registrations = re.findall(r"\bregister_image\s*\(\s*([^,]+)\s*,", main)
registrations = [arg.strip() for arg in registrations if arg.strip() != "int id"]
if not registrations:
    sys.exit("ERROR: no register_image calls found in kernel/main.c")
ids = []
for arg in registrations:
    if not re.fullmatch(r"\d+", arg):
        sys.exit(f"ERROR: nonliteral program ID {arg!r}; update capacity check")
    ids.append(int(arg))
if len(ids) != len(set(ids)):
    sys.exit("ERROR: duplicate RAM application IDs")
invalid = [i for i in ids if not (1 <= i < capacity)]
if invalid:
    sys.exit(f"ERROR: program IDs {invalid} outside valid range 1..{capacity-1}")
print(f"PASS: {len(ids)} RAM applications; max ID {max(ids)} < PROGRAM_CAPACITY {capacity}")
