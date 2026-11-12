#!/usr/bin/env python3
"""Guard fixed-width command output and PTY documentation after display cleanup."""
from pathlib import Path
r=Path(__file__).resolve().parents[2]
ps=(r/'userland/apps/ps.c').read_text()
k=(r/'kernel/kernel.c').read_text()
trace=(r/'userland/apps/trace.c').read_text()
i2c=(r/'userland/apps/i2cctl.c').read_text()
assert 'Slot 1 is not exposed via ptmx' not in k
assert 'ptmx allocates either free slot' in k
assert 'int fields=sscanf(line' in ps
assert 'show_task(line)' in ps
assert 'char line[192],buf[64]' in ps
assert 'static const char header[] = "PID  NAME' in k
assert 'i2cctl xfer ADDR READ_COUNT [BYTE...]' in i2c
assert 'trace run [-s|-a] command [args...]' in trace
for lang in ('ja','en'):
    d=(r/f'docs/{lang}/reference/INTERNAL.md').read_text()
    assert '/proc/tasks' in d and 'CPU_TICKS' in d and 'DISPATCH' in d
print('PASS: ps narrow layout and preserved detailed proc, PTY comment, help text')
