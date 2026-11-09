#!/usr/bin/env python3
"""Guard raw exit status propagation and PPID append-only output."""
from pathlib import Path
r=Path(__file__).resolve().parents[2]
k=(r/'kernel/kernel.c').read_text()
s=(r/'userland/apps/shell.c').read_text()
m=(r/'userland/apps/mcpd.c').read_text()
assert 'int pid, parent_pid, status;' in k
assert 'exited[pos].status=status & 255;' in k
assert '*t->wait_status=status;' in k
assert '*status=exit_status;' in k
assert 'static const char header[] = "PID  NAME              STATE    TTY      SLOT  MEMORY             SP        PPID\\n";' in k
assert 't->sp,t->parent_pid);' in k
assert 'return command_status;' in s
assert 'if(!error && done && status!=0) error=1;' in m
for locale in ('ja','en'):
    assert 'PPID' in (r/f'docs/{locale}/reference/INTERNAL.md').read_text()
print('PASS: exit status, shell/MCP propagation and PPID')
