#!/usr/bin/env python3
"""Ensure /proc/cpu cannot alias an unrelated VFS device ID."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[2]
kernel = (root / "kernel/kernel.c").read_text()
fixed = (root / "include/device_names.h").read_text()

def ids(src):
    return {name: int(value) for name, value in
            re.findall(r"\b(DEV_[A-Z0-9_]+)\s*=\s*(\d+)\s*[,}]", src)}

internal = ids(kernel)
external = ids(fixed)
cpu = internal["DEV_PROC_CPU"]
assert cpu not in external.values(), [name for name, value in external.items() if value == cpu]
assert cpu not in (value for name, value in internal.items() if name != "DEV_PROC_CPU")
# The CPU virtual file must not be swallowed by an earlier range handler.
assert not (internal["DEV_TMP_FIRST"] <= cpu < internal["DEV_TMP_FIRST"] + 11)
assert not (internal["DEV_PROC_TASK_BASE"] <= cpu < internal["DEV_PROC_TASK_BASE"] + 6)
assert not (internal["DEV_SYMS_USER_BASE"] < cpu < internal["DEV_SYMS_USER_BASE"] + 64)
assert 'if(dev==DEV_NET_PACKET)' in kernel
assert 'if(proc_device(dev))' in kernel
print("PASS: /proc/cpu uses an unambiguous VFS device ID")
