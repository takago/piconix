#!/usr/bin/env python3
"""Verify that full listings in the bilingual socket tutorial match bundled examples."""
from pathlib import Path
import re
ROOT = Path(__file__).resolve().parents[2]
examples = {
    "udp_once.c": "/* Pico W: send one UDP datagram",
    "tcp_get.c": "/* Pico W: minimal TCP client",
}
for lang in ("ja", "en"):
    doc = (ROOT / "docs" / lang / "development/apps/c/NETWORK-PROGRAMMING.md").read_text(encoding="utf-8")
    blocks = re.findall(r"```c\n(.*?)\n```", doc, re.S)
    for name, marker in examples.items():
        source = (ROOT / "userland/examples/c/network" / name).read_text(encoding="utf-8").strip()
        matching = [block.strip() for block in blocks if block.startswith(marker)]
        assert matching == [source], f"{lang}: stale {name} listing"
    host = (ROOT / "host/udp_echo.py").read_text(encoding="utf-8").strip()
    assert host in doc, f"{lang}: stale host UDP echo listing"
print("PASS: bilingual network example listings match source files")
