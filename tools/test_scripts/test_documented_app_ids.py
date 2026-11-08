#!/usr/bin/env python3
"""Check that documented example application IDs are distinct and currently available."""
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "kernel/main.c").read_text(encoding="utf-8")
registered = {int(x) for x in re.findall(r'register_image\(\s*(\d+)\s*,', source)}
assert registered == set(range(1, 52)), f"Unexpected stock app IDs: {sorted(registered)}"
examples = {
    "greet": 52,
    "udp_once": 53,
    "tcp_get": 54,
    "labuptime": 55,
}
assert len(set(examples.values())) == len(examples)
assert registered.isdisjoint(examples.values())
for lang in ("en", "ja"):
    for filename, names in (
        ("development/apps/c/PROGRAMMING.md", ["greet"]),
        ("development/apps/c/NETWORK-PROGRAMMING.md", ["udp_once", "tcp_get"]),
        ("development/syscalls/SYSCALL-DEVELOPMENT.md", ["labuptime"]),
    ):
        doc = (ROOT / "docs" / lang / filename).read_text(encoding="utf-8")
        for name in names:
            pattern = rf'register_image\(\s*{examples[name]}\s*,\s*"{name}"'
            assert re.search(pattern, doc), f"{lang}/{filename}: stale registration for {name}"
print("PASS: example IDs are distinct and not used by stock firmware")
