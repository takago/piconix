#!/usr/bin/env python3
"""Check internal /etc and /tmp path capacity (31 bytes + NUL)."""
import importlib.util
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "kernel/kernel.c").read_text()
assert "#define TMP_NAME_CAP 32" in source
assert "char name[TMP_NAME_CAP]" in source
assert "len<TMP_NAME_CAP-5" in source
spec = importlib.util.spec_from_file_location("generate_initial_fs", root / "tools/generate_initial_fs.py")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
with tempfile.TemporaryDirectory() as temp:
    base = Path(temp)
    (base / "etc_picow").mkdir()
    (base / "etc_picow" / ("x" * 27)).write_bytes(b"OK")  # 4+27=31
    files = mod.generate(base, "pico_w", base.parent / "path32-ok.h")
    assert len(next(iter(files))) == 31
    (base / "etc_picow" / ("x" * 28)).write_bytes(b"BAD")  # 4+28=32
    try:
        mod.generate(base, "pico_w", base.parent / "path32-bad.h")
    except ValueError as exc:
        assert "VFS name too long" in str(exc)
    else:
        raise AssertionError("32-byte name accepted")
print("PASS: RAM path length accepts 31 bytes and rejects 32 bytes")
