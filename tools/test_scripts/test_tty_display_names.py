#!/usr/bin/env python3
"""Ensure task TTY names in /proc/tasks match the exposed device nodes."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
kernel = (root / "kernel/kernel.c").read_text()
start = kernel.index("NO_TRACE static int proc_tasks_read(")
end = kernel.index("NO_TRACE static int proc_fb_read(", start)
proc = kernel[start:end]
for mapping in ('t->tty==0?"ttyV0"', 't->tty==2?"ttyN0"',
                't->tty==PTY_TTY?"pts/0"', 't->tty==PTY_TTY1?"pts/1"',
                't->tty==CDC_VCONSOLE_TTY?"ttyV1"'):
    assert mapping in proc, mapping
assert 't->tty==2?"wifi"' not in proc
assert 'ttyN0' in (root / 'include/device_names.h').read_text()
trace = (root / "userland/apps/trace.c").read_text()
assert '"ttyN0"' in trace
print("PASS: /proc/tasks, trace and /dev/ttyN0 use consistent network TTY names")
