"""Static contract checks for the planned RP2040 pipe implementation.

Run after implementing pipe; fails until the API and shell are wired up.
"""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[2]
kernel = (root / 'kernel/kernel.c').read_text()
header = (root / 'include/piconix.h').read_text()
shell = (root / 'userland/apps/shell.c').read_text()
checks = {
    'public pipe API': bool(re.search(r'\b(?:os_)?pipe\s*\(', header)),
    'pipe syscall dispatch': bool(re.search(r'\bSYS_PIPE\b', kernel)),
    'pipe buffer storage': bool(re.search(r'\bpipe(?:s|_buf|_buffer)?\b', kernel)),
    'pipe shell operator': bool(re.search(r"\*\w+\s*==\s*'\|'|strchr\([^\n]*'\|'", shell)),
}
for label, ok in checks.items():
    print(f"{'PASS' if ok else 'PENDING'}: {label}")
if not all(checks.values()):
    raise SystemExit('Pipe integration incomplete (expected before implementation)')
