#!/usr/bin/env python3
"""Check shell Ctrl+L redraw and boot banner without an ARM toolchain."""
from pathlib import Path
s = (Path(__file__).resolve().parents[1] / "userland/shell.c").read_text()
assert 'if(c==12)' in s
assert 'put("\\033[2J\\033[H")' in s
assert 'shell_redraw(line,len,cursor);' in s.split('if(c==12)', 1)[1].split('continue;', 1)[0]
assert 'PicoNIX-2026-10-30 (TAKAGO_LAB.)' in s
assert 'PicoNIX-2026-10-26' not in s
print("PASS: Ctrl+L redraw and 2026-10-30 banner")
