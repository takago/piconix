#!/usr/bin/env python3
"""Keep net_poll_in_progress linkable without CYW43/lwIP on regular Pico."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix="piconix_nonwifi_") as temp:
    obj = Path(temp) / "net.o"
    subprocess.run([
        "cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-DPICONIX_WIFI=0", "-I", str(root / "include"),
        "-I", str(root / "kernel"), "-c", str(root / "kernel" / "net.c"),
        "-o", str(obj),
    ], check=True)
    symbols = subprocess.check_output(["nm", "-g", str(obj)], text=True)
    assert " T net_poll_in_progress" in symbols
print("PASS: non-Wi-Fi build exports net_poll_in_progress")
