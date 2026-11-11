#!/usr/bin/env python3
"""Check that documented example application IDs are distinct and currently available."""
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "kernel/main.c").read_text(encoding="utf-8")
pairs = [(int(i), name) for i, name in re.findall(
    r'register_image\(\s*(\d+)\s*,\s*"([^"]+)"', source)]
registered = {i for i, _ in pairs}
assert len(registered) == len(pairs), f"Duplicate stock app IDs: {pairs}"
cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
cmake_names = set(re.search(r"foreach\(APP IN ITEMS (.*?)\)", cmake, re.S).group(1).split())
embedded = {({"sh": "shell", "as": "asm"}.get(name, name)) for _, name in pairs}
assert embedded == cmake_names, f"Registration mismatch: {sorted(embedded ^ cmake_names)}"
assert len(pairs) == len(cmake_names), "Duplicate embedded name or registration"
examples = {
    "greet": 52,
    "udp_once": 53,
    "tcp_get": 54,
    "labuptime": 55,
}
assert len(set(examples.values())) == len(examples)
assert registered.isdisjoint(examples.values())
for lang in ("en", "ja"):
    index = (ROOT / "docs" / lang / "reference/commands/README.md").read_text(encoding="utf-8")
    assert f"{len(cmake_names)}" in index.splitlines()[2], f"{lang}: incorrect command count"
    for name in (name if name not in {"shell", "asm"} else {"shell": "sh", "asm": "as"}[name] for name in cmake_names):
        assert f"[`{name}`]" in index, f"{lang}: command missing from index: {name}"
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
