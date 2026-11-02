#!/usr/bin/env python3
"""Reject accidental collisions between net diagnostic and fixed device IDs."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
kernel = (root / "kernel/kernel.c").read_text()
shared = (root / "include/device_names.h").read_text()

def ids(text):
    return {k: int(v) for k, v in re.findall(r"\b(DEV_[A-Z0-9_]+)\s*=\s*(\d+)", text)}

k, common = ids(kernel), ids(shared)
for name in ("DEV_PROC_AP", "DEV_PROC_ARP"):
    value = k[name]
    collisions = [n for n, x in common.items() if x == value]
    assert not collisions, f"{name}={value} collides with {collisions}"
assert k["DEV_PROC_AP"] != k["DEV_PROC_ARP"]
assert all(not (k["DEV_TMP_FIRST"] <= k[n] < k["DEV_TMP_FIRST"] + 11) for n in ("DEV_PROC_AP", "DEV_PROC_ARP"))
assert all(not (200 <= k[n] < 265 or 240 <= k[n] < 244) for n in ("DEV_PROC_AP", "DEV_PROC_ARP"))
print("PASS: net diagnostic device IDs do not collide with existing fixed devices")
