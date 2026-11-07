"""Regression: watch flushes ANSI diff output even if no newline is emitted."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
s = (root / 'userland/apps/watch.c').read_text()
assert re.search(r'redraw\(&frames\[old\],\s*&frames\[next\],\s*first_frame\);\s*/\*.*?\*/\s*if\s*\(fflush\(stdout\)<0\)', s, re.S)
print('PASS: watch flushes output after each diff redraw')
